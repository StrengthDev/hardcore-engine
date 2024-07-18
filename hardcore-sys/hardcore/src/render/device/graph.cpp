#include <pch.hpp>

#include "graph.hpp"

namespace hc::render::device {
    /**
     * @brief Generate the next ID for some item.
     *
     * @param seed The current seed to use for generation.
     * @return The generated ID.
     */
    GraphItemID next_id(GraphItemID &seed) noexcept {
        // Unsure of this is the best way to generate IDs, but in practice there should no collisions at least.
        return seed++;
    }

    Graph Graph::create(u32 graphics_queue_family, u32 compute_queue_family, u32 transfer_queue_family) {
        Sz command_queues;
        u8 graphics_idx, compute_idx, transfer_idx;
        if (graphics_queue_family == compute_queue_family && compute_queue_family == transfer_queue_family) {
            // Only 1 generic queue family
            command_queues = 1;
            graphics_idx = 0;
            compute_idx = 0;
            transfer_idx = 0;
        } else if (graphics_queue_family != compute_queue_family && compute_queue_family != transfer_queue_family
                   && graphics_queue_family != transfer_queue_family) {
            // 3 dedicated queue families
            command_queues = 3;
            graphics_idx = 0;
            compute_idx = 1;
            transfer_idx = 2;
        } else {
            command_queues = 2;
            if (graphics_queue_family == compute_queue_family) {
                // Dedicated transfer family
                graphics_idx = 0;
                compute_idx = 0;
                transfer_idx = 1;
            } else if (compute_queue_family == transfer_queue_family) {
                // Weird case, unsure if any hardware matches this
                graphics_idx = 0;
                compute_idx = 1;
                transfer_idx = 1;
            } else {
                // Dedicated compute family, probably doesn't exist, as it makes no sense to have a dedicated compute
                // queue and no dedicated transfer queue
                graphics_idx = 0;
                compute_idx = 1;
                transfer_idx = 0;
            }
        }

        return Graph(graphics_idx, compute_idx, transfer_idx, command_queues);
    }

    Graph::Graph(u8 graphics_idx, u8 compute_idx, u8 transfer_idx, Sz command_queues) :
            graphics_idx(graphics_idx), compute_idx(compute_idx), transfer_idx(transfer_idx),
            commands(command_queues) {}

    bool Graph::validate_graph() const noexcept {
        // Check for missing dependencies

        std::unordered_set<GraphItemID> node_dependencies;
        std::unordered_set<GraphItemID> resource_dependencies;
        for (const auto &[id, node]: this->nodes) {
            for (const InputResource &input: node.inputs) {
                resource_dependencies.insert(input.id);
                if (input.origin)
                    node_dependencies.insert(*input.origin);
            }
        }
        for (const GraphItemID id: node_dependencies) {
            if (!this->nodes.contains(id)) {
                HC_ERROR("Invalid device graph, node dependency does not exist "
                         "(this may happen because an operation call was destroyed while still being used)");
                return false;
            }
        }
        for (const GraphItemID id: resource_dependencies) {
            if (!this->nodes.contains(id)) {
                HC_ERROR("Invalid device graph, resource dependency does not exist "
                         "(this may happen because a resource was destroyed while still being used)");
                return false;
            }
        }

        return true;
    }

    std::pair<std::unordered_set<GraphItemID>, std::vector<GraphItemID>> Graph::filter_unused_nodes() const noexcept {
        std::vector<GraphItemID> dependency_stack;
        std::unordered_set<GraphItemID> output_dependencies;
        // Can assume that this will only contain unique root nodes, because the stack will never have duplicates
        std::vector<GraphItemID> root_nodes;
        for (auto const &[resource_id, node_id]: this->outputs) {
            bool is_root = true;
            auto node = this->nodes.find(node_id);
            HC_ASSERT(node != this->nodes.end(), "The output should exist within the graph");
            for (const InputResource &input: node->second.inputs) {
                if (input.origin) {
                    is_root = false;
                    auto [it, inserted] = output_dependencies.insert(*input.origin);
                    if (inserted)
                        dependency_stack.push_back(*input.origin);
                }
            }
            output_dependencies.insert(node_id);
            if (is_root)
                root_nodes.push_back(node_id);
        }
        while (!dependency_stack.empty()) {
            const GraphItemID current_node_id = dependency_stack.back();
            dependency_stack.pop_back();

            bool is_root = true;
            auto node = this->nodes.find(current_node_id);
            HC_ASSERT(node != this->nodes.end(), "The dependency should exist within the graph");
            for (const InputResource &input: node->second.inputs) {
                if (input.origin) {
                    is_root = false;
                    auto [it, inserted] = output_dependencies.insert(*input.origin);
                    if (inserted)
                        dependency_stack.push_back(*input.origin);
                }
            }
            if (is_root)
                root_nodes.push_back(current_node_id);
        }
        HC_ASSERT(output_dependencies.size() <= this->nodes.size(), "The number of output dependencies should be the "
                                                                    "same or lower than the number of nodes in the "
                                                                    "graph");
        if (output_dependencies.size() != this->nodes.size()) {
            HC_WARN("There are " << (this->nodes.size() - output_dependencies.size())
                                 << " unused operations within the device graph");
        }

        return {output_dependencies, root_nodes};
    }

    GraphResult Graph::compile() {
        HC_INFO_SPAN("Compile device graph");
        if (this->is_compiled())
            return GraphResult::Success;

        if (!this->validate_graph())
            return GraphResult::Invalid;

        auto [output_dependencies, root_nodes] = this->filter_unused_nodes();

        std::queue<GraphItemID> pending_queue;
        std::unordered_set<GraphItemID> pending_set;
        std::unordered_set<GraphItemID> pushed_nodes;
        std::vector<GraphItemID> push_batch;

        for (const GraphItemID node: root_nodes) {
            auto [it, inserted] = pending_set.insert(node);
            if (inserted)
                pending_queue.push(node);
        }

        // Nodes are pushed in batches to the command queues, each node in a batch should be independent of each other
        while (pushed_nodes.size() < output_dependencies.size()) {
            Sz check_count = pending_queue.size();
            while (0 < check_count) {
                GraphItemID node = pending_queue.front();
                pending_queue.pop();

                // Check if node is ready to be pushed (all of its dependencies have been pushed)
                bool ready = true;
                for (const InputResource &input: this->nodes[node].inputs) {
                    if (input.origin && !pushed_nodes.contains(*input.origin)) {
                        ready = false;
                        break;
                    }
                }
                if (ready) {
                    pending_set.erase(node);
                    push_batch.push_back(node);
                    for (const OutputResource &output: this->nodes[node].outputs) {
                        for (const GraphItemID dependent: output.dependents) {
                            // Only nodes that do meaningful work are pushed
                            if (output_dependencies.contains(dependent)) {
                                auto [it, inserted] = pending_set.insert(dependent);
                                if (inserted)
                                    pending_queue.push(dependent);
                            }
                        }
                    }
                } else {
                    pending_queue.push(node);
                }
                check_count--;
            }

            for (const GraphItemID node: push_batch) {
                pushed_nodes.insert(node);
                // TODO add commands
            }

            push_batch.clear();
        }

        return GraphResult::Success;
    }

    bool Graph::is_compiled() const noexcept {
        bool compiled = false;
        for (const auto &queue: this->commands) {
            if (!queue.empty()) {
                compiled = true;
                break;
            }
        }
        return compiled || this->nodes.empty();
    }

    void Graph::clear_commands() noexcept {
        for (auto &queue: this->commands)
            queue.clear();
    }

    GraphResult Graph::record() const noexcept {
        HC_INFO_SPAN("Record command buffers");
        if (!this->is_compiled()) {
            return GraphResult::NotCompiled;
        }

        // TODO do NOT forget: dedicated transfer queue for host-device transfers, otherwise use the resource owning family

        return GraphResult::Success;
    }

    std::size_t Graph::OutputHash::operator()(const std::pair<GraphItemID, GraphItemID> &output) const noexcept {
        return output.first ^ reverse_bits(output.second);
    }
}
