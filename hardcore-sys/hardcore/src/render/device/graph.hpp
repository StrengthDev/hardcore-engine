#pragma once

#include <unordered_map>
#include <optional>

#include <render/renderer.h>
#include <render/device/memory/reference.hpp>
#include <util/number.hpp>
#include <util/bank.hpp>

#include "destruction_mark.hpp"

namespace hc::render::device {
    enum class GraphResult : u8 {
        Success = 0,
        NotCompiled,
        Invalid,
    };

    /**
     * @brief A node's input resource.
     */
    struct InputResource {
        GraphItemID id = std::numeric_limits<GraphItemID>::max(); //!< The ID of the resource in the graph.
        std::optional<GraphItemID> origin; //!< The key of the graph node which wrote to this resource, if any did.
    };

    /**
     * @brief A node's output resource.
     */
    struct OutputResource {
        GraphItemID id = std::numeric_limits<GraphItemID>::max(); //!< The ID of the resource in the graph.
        std::vector<GraphItemID> dependents; //!< The list of nodes depending on this resource.
    };

    enum class NodeType : u8 {
        Compute,
        Raster,
        RayTracing,
    };

    /**
     * @brief A node in a graph.
     *
     * This represents a call or dispatch of a device operation, such as a compute pipeline call.
     */
    struct Node {
        NodeType type; //!< The type of operation this node represents.
        std::vector<InputResource> inputs; //!< The list of resource inputs for this node.
        std::vector<OutputResource> outputs; //!< The list of resource outputs for this node.
    };

    /**
     * @brief A graph resource.
     */
    struct Resource {
        VkBuffer buffer; //!< The buffer where this resource is located.
        VkDeviceSize size; //!< The size in bytes of this resource.
        VkDeviceSize frame_pad; //!< The value used to calculate the offset to the currently active area of this resource's buffer.
        VkDeviceSize offset; //!< The offset to this resource within its buffer's currently active area. (Includes alignment padding)
        struct Logistics {
            u64 pool; //!< The id of the memory pool this resource is allocated in. (Excludes alignment padding)
            VkDeviceSize offset; //!< The offset of this resource inside its respective memory pool.
            VkBufferUsageFlags flags; //!< The usage flags of this resource.
            bool dynamic; //!< Flag indicating if the resource is dynamic.
        } logistics; //!< Data used for purposes of cleanup.
    };

    /**
     * @brief An acyclic computational graph.
     */
    class Graph {
    public:
        Graph() = default;

        static Graph create(u32 graphics_queue_family, u32 compute_queue_family, u32 transfer_queue_family);

        Graph(Graph &&other) noexcept = default;

        Graph &operator=(Graph &&other) noexcept = default;

        Graph(const Graph &) = delete;

        Graph &operator=(const Graph &) = delete;

        /**
         * @brief Compile the graph into an optimized set of commands.
         *
         * No-op if no significant changes were made to the graph.
         * This function MUST be called before `record`.
         * If the logical flow of the graph is unsound, an error is returned.
         *
         * @return GraphResult::Success if the graph was successfully compiled, otherwise an appropriate error value.
         */
        [[nodiscard]] GraphResult compile();

        /**
         * @brief Check if the graph has already been compiled.
         *
         * @return *true* if the is compiled, *false* otherwise.
         */
        [[nodiscard]] bool is_compiled() const noexcept;

        /**
         * @brief Clear the current command compilation.
         *
         * The graph must be re-compiled, using `compile`, after calling this function, before `record` is called.
         */
        void clear_commands() noexcept;

        [[nodiscard]] GraphResult record() const noexcept;

        [[nodiscard]] u64 add_resource(const memory::Ref &ref, bool dynamic);

        [[nodiscard]] ResourceDestructionMark remove_resource(u64 id);

    private:
        Graph(u8 graphics_idx, u8 compute_idx, u8 transfer_idx, Sz command_queues);

        /**
         * @brief Verify if the graph is malformed.
         *
         * @param nodes The nodes composing the graph.
         * @return *true* if the graph structure is correct, *false* otherwise.
         */
        [[nodiscard]] bool validate_graph() const noexcept;

        /**
         * @brief Filter unused nodes in the graph.
         *
         * A node is considered unused if it does not contribute in any way to any output of the graph.
         *
         * @return A pair containing the set of used nodes, and a list of relevant root nodes (also included within
         * the used nodes, it is also guaranteed that there are no duplicate root nodes in the list).
         */
        [[nodiscard]] std::pair<std::unordered_set<GraphItemID>, std::vector<GraphItemID>>
        filter_unused_nodes() const noexcept;

        // It is assumed the nodes in the graph never form a cycle

        Bank<Node> nodes; //!< The collection of nodes that compose the graph.
        Bank<Resource> resources; //!< The collection of graph resources, used by nodes.

        u8 graphics_idx = std::numeric_limits<u8>::max(); //!< The index of the graphics command list in `commands`.
        u8 compute_idx = std::numeric_limits<u8>::max(); //!< The index of the compute command list in `commands`.
        u8 transfer_idx = std::numeric_limits<u8>::max(); //!< The index of the transfer command list in `commands`.

        std::vector<std::vector<void *>> commands; //!< A compiled list of commands.

        /**
         * @brief Custom hash function for the output type.
         */
        struct OutputHash {
            std::size_t operator()(const std::pair<GraphItemID, GraphItemID> &output) const noexcept;
        };

        /**
         * @brief The set of graph resources which are output by the graph.
         *
         * Each item is a pair of ID's, the first being the resource ID and the second the last node that writes to it.
         */
        std::unordered_set<std::pair<GraphItemID, GraphItemID>, OutputHash> outputs;
    };

}
