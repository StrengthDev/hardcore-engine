#include <pch.hpp>

#include <util/flow.hpp>
#include <util/number.hpp>

#include "graph.hpp"

namespace hc::render::device {
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

    void Graph::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        render_passes.destroy(fn_table, device);
    }

    Graph::Graph(u8 graphics_idx, u8 compute_idx, u8 transfer_idx, Sz command_queues)
        : graphics_idx(graphics_idx),
        compute_idx(compute_idx),
        transfer_idx(transfer_idx),
        commands(command_queues) {}

    std::expected<u64, Error> Graph::insert_node(
        std::vector<ResourceRef>&& inputs,
        std::vector<ResourceRef>&& outputs,
        std::vector<u64>&& dependencies
    ) {
        for (auto const& input : inputs) {
            if (!this->resources.contains(input.id)) {
                HC_ERROR("The provided resource input does not exist: " << input.id);
                return Error(HCError_NoSuchResource);
            }
        }

        std::vector<std::reference_wrapper<Node>> dependency_nodes;
        dependency_nodes.reserve(dependencies.size());

        for (auto const dependency_id : dependencies) {
            if (!this->nodes.contains(dependency_id)) {
                HC_ERROR("The provided execution graph node does not exist: " << dependency_id);
                return Error(HCError_NoSuchNode);
            }

            if (this->pruned_node_ids.contains(dependency_id)) {
                HC_ERROR("The provided execution graph node has deleted dependencies: " << dependency_id);
                return Error(HCError_PrunedNode);
            }

            dependency_nodes.emplace_back(nodes[dependency_id]);
        }

        this->dirty = true;

        auto id = nodes.insert(
            {
                .type = NodeType::Compute,
                .inputs = std::move(inputs),
                .outputs = std::move(outputs),
                .dependencies = std::move(dependencies),
                .dependents = {},
            }
        );

        for (auto& dependency_node : dependency_nodes) {
            dependency_node.get().dependents.push_back(id);
        }

        return id;
    }

    void Graph::remove_node(u64 id) {
        if (!this->nodes.contains(id)) {
            HC_WARN("Node does not exist: " << id);
            return;
        }

        this->prune_node(id);

        this->pruned_node_ids.erase(id);
        this->nodes.erase(id);
    }

    void Graph::prune_node(u64 prune_id) {
        std::stack<u64> prune_stack;
        prune_stack.push(prune_id);

        // Prune the node and all its dependents recursively
        while (!prune_stack.empty()) {
            auto const id = prune_stack.top();
            prune_stack.pop();

            if (!this->pruned_node_ids.contains(id)) {
                this->pruned_node_ids.insert(id);
                this->dirty = true;

                auto node = this->nodes[id];

                for (auto const dependent_id : node.dependents) {
                    prune_stack.push(dependent_id);
                }

                for (auto const dependency_id : node.dependencies) {
                    std::erase(this->nodes[dependency_id].dependents, id);
                }

                node.inputs.clear();
                node.dependents.clear();
                node.dependencies.clear();
            }
        }
    }

    u64 Graph::insert_resource(buffer::Buffer&& resource) {
        return this->resources.insert(std::move(resource));
    }

    void Graph::remove_resource(u64 id) {
        if (!this->resources.contains(id)) {
            HC_WARN("Resource does not exist: " << id);
            return;
        }

        for (auto& [node_id, node] : this->nodes) {
            if (std::ranges::find_if(node.inputs, [id](auto input) { return input.id == id; }) != node.inputs.end()) {
                this->prune_node(node_id);
            }
        }

        resources.erase(id);
    }

    std::expected<std::optional<u64>, Error> Graph::get_dependency_node(HCDependency const& dependency) const noexcept {
        if (!dependency.handle) {
            return std::nullopt;
        }

        switch (dependency.type) {
        case HCDependencyType_RenderPass:
            auto const* render_pass = static_cast<HCRenderPass const*>(dependency.handle);

            if (auto const it = this->render_pass_datas.find(render_pass->id); it == this->render_pass_datas.end()) {
                HC_ERROR("The provided render pass does not exist");
                return Error(HCError_InvalidParams);
            } else {
                return it->second.nodes.back();
            }
        }

        HC_ERROR("Invalid dependency type");
        return Error(HCError_InvalidParams);
    }

    std::expected<void, Error> Graph::compile() {
        HC_INFO_SPAN("Compile device graph");

        return {};
    }

    bool Graph::is_compiled() const noexcept {
        bool compiled = false;
        for (const auto& queue : this->commands) {
            if (!queue.empty()) {
                compiled = true;
                break;
            }
        }
        return compiled || this->nodes.empty();
    }

    void Graph::clear_commands() noexcept {
        for (auto& queue : this->commands) {
            queue.clear();
        }
    }

    std::expected<void, Error> Graph::record() const noexcept {
        HC_ASSERT(this->is_compiled(), "Graph must be compiled before recording");

        HC_INFO_SPAN("Record command buffers");

        // TODO do NOT forget: dedicated transfer queue for host-device transfers, otherwise use the resource owning family

        return {};
    }

    u64 Graph::add_resource(buffer::Buffer&& buffer) {
        return this->resources.insert(std::move(buffer));
    }

    u64 Graph::add_resource(buffer::DynamicBuffer&& buffer) {
        return this->dynamic_resources.insert(std::move(buffer));
    }

    buffer::Buffer Graph::remove_resource_b(u64 id) {
        return this->resources.erase(id);
    }

    buffer::DynamicBuffer Graph::remove_resource_bd(u64 id) {
        return this->dynamic_resources.erase(id);
    }

    u64 Graph::add_texture(texture::Texture&& texture) {
        return this->textures.insert(std::move(texture));
    }

    texture::Texture Graph::remove_texture(u64 id) {
        return this->textures.erase(id);
    }

    std::expected<u64, Error> Graph::create_render_pass(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::span<HCSubpass const> const& subpasses,
        UserPredicate<Sz>&& predicate
    ) {
        auto render_pass_result = this->render_passes.create_render_pass(fn_table, device, subpasses, this->textures);
        if (!render_pass_result) {
            return render_pass_result.error();
        }
        u64 render_pass_id = *render_pass_result;

        std::vector<u64> subpass_nodes;
        subpass_nodes.reserve(subpasses.size());

        for (auto const& subpass : subpasses) {
            std::vector<ResourceRef> inputs;
            std::vector<ResourceRef> outputs;
            std::vector<u64> dependencies;

            if (!subpass_nodes.empty()) {
                dependencies.emplace_back(subpass_nodes.back());
            }

            for (auto const& input : std::span(subpass.inputs, subpass.input_count)) {
                auto input_node = this->get_dependency_node(input.dependency);
                if (!input_node) {
                    this->remove_nodes(subpass_nodes);
                    this->render_passes.destroy_render_pass(fn_table, device, render_pass_id);

                    return input_node.error();
                }

                if (*input_node) {
                    dependencies.emplace_back(**input_node);
                }

                inputs.push_back({.id = input.texture_id, .type = ResourceType::Texture});
            }

            for (auto const& output : std::span(subpass.outputs, subpass.output_count)) {
                auto output_node = this->get_dependency_node(output.dependency);
                if (!output_node) {
                    this->remove_nodes(subpass_nodes);
                    this->render_passes.destroy_render_pass(fn_table, device, render_pass_id);

                    return output_node.error();
                }

                if (*output_node) {
                    dependencies.emplace_back(**output_node);
                }

                outputs.push_back({.id = output.texture_id, .type = ResourceType::Texture});
            }

            if (subpass.depth_stencil_attachment) {
                auto depth_stencil_node = this->get_dependency_node(subpass.depth_stencil_attachment->dependency);
                if (!depth_stencil_node) {
                    this->remove_nodes(subpass_nodes);
                    this->render_passes.destroy_render_pass(fn_table, device, render_pass_id);

                    return depth_stencil_node.error();
                }

                if (*depth_stencil_node) {
                    dependencies.emplace_back(**depth_stencil_node);
                }

                outputs.push_back({.id = subpass.depth_stencil_attachment->texture_id, .type = ResourceType::Texture});
            }

            auto node_result = this->insert_node(std::move(inputs), std::move(outputs), std::move(dependencies));
            if (!node_result) {
                this->remove_nodes(subpass_nodes);
                this->render_passes.destroy_render_pass(fn_table, device, render_pass_id);

                return node_result.error();
            }

            subpass_nodes.emplace_back(*node_result);
        }

        this->render_pass_datas.emplace(
            render_pass_id,
            RenderPassGraphData{.nodes = std::move(subpass_nodes), .predicate = std::move(predicate)}
        );

        return render_pass_id;
    }

    void Graph::destroy_render_pass(VolkDeviceTable const& fn_table, VkDevice device, u64 id) {
        this->render_pass_datas.erase(id);
        this->render_passes.destroy_render_pass(fn_table, device, id);
    }
}
