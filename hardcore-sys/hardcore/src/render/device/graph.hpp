#pragma once

#include <render/device/memory/reference.hpp>
#include "render/render_pass/render_pass_bank.hpp"
#include "render/resource/buffer.hpp"
#include "render/resource/texture.hpp"

#include <render/ops/common.h>
#include <render/ops/render_pass.h>

#include <util/number.hpp>
#include <util/bank.hpp>
#include <util/user_predicate.hpp>

#include <span>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

namespace hc::render::device {
    class Graph;

    class GraphCompilation {
    public:
        [[nodiscard]]
        static std::expected<GraphCompilation, Error> create(Graph const& graph) noexcept;

        void record_commands();

    private:
        GraphCompilation() = default;
    };

    struct Command {
        u64 id;
    };

    struct RenderPass {};

    enum class ResourceType: u8 {
        Texture,
    };

    struct ResourceRef {
        u64 id = std::numeric_limits<u64>::max();
        ResourceType type;
    };

    enum class NodeType : u8 {
        Compute,
        Raster,
        RayTracing,
    };

    struct Node {
        NodeType type;
        std::vector<ResourceRef> inputs;
        std::vector<ResourceRef> outputs;
        std::vector<u64> dependencies;
        std::vector<u64> dependents;
    };

    /**
    * @brief A graph resource.
    */
    struct Resource {
        VkBuffer buffer; //!< The buffer where this resource is located.
        VkDeviceSize size; //!< The size in bytes of this resource.
        VkDeviceSize frame_pad; //!< The value used to calculate the offset to the currently active area of this resource's buffer.
        VkDeviceSize offset; //!< The offset to this resource within its buffer's currently active area. (Includes alignment padding)
        struct Memory {
            u64 pool; //!< The id of the memory pool this resource is allocated in. (Excludes alignment padding)
            VkDeviceSize offset; //!< The offset of this resource inside its respective memory pool.
            u32 flags; //!< The usage flags of this resource.
            bool dynamic; //!< Flag indicating if the resource is dynamic.
        } memory; //!< Data used for cleanup.
    };

    /**
    * @brief An acyclic computational graph.
    */
    class Graph {
    public:
        Graph() = default;

        static Graph create(u32 graphics_queue_family, u32 compute_queue_family, u32 transfer_queue_family);

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

        Graph(Graph&& other) noexcept = default;

        Graph& operator=(Graph&& other) noexcept = default;

        Graph(const Graph&) = delete;

        Graph& operator=(const Graph&) = delete;

        /**
        * @brief Compile the graph into an optimized set of commands.
        *
        * No-op if no significant changes were made to the graph.
        * This function MUST be called before `record`.
        * If the logical flow of the graph is unsound, an error is returned.
        *
        * @return GraphResult::Success if the graph was successfully compiled, otherwise an appropriate error value.
        */
        [[nodiscard]] std::expected<void, Error> compile();

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

        [[nodiscard]] std::expected<void, Error> record() const noexcept;

        [[nodiscard]] u64 add_resource(buffer::Buffer&& buffer);
        [[nodiscard]] u64 add_resource(buffer::DynamicBuffer&& buffer);

        [[nodiscard]] buffer::Buffer remove_resource_b(u64 id);
        [[nodiscard]] buffer::DynamicBuffer remove_resource_bd(u64 id);

        [[nodiscard]] u64 add_texture(texture::Texture&& texture);

        [[nodiscard]] texture::Texture remove_texture(u64 id);

        [[nodiscard]]
        std::expected<u64, Error> create_render_pass(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::span<HCSubpass const> const& subpasses,
            UserPredicate<Sz>&& predicate
        );

        void destroy_render_pass(VolkDeviceTable const& fn_table, VkDevice device, u64 id);

    private:
        Graph(u8 graphics_idx, u8 compute_idx, u8 transfer_idx, Sz command_queues);

        [[nodiscard]]
        std::expected<u64, Error> insert_node(
            std::vector<ResourceRef>&& inputs,
            std::vector<ResourceRef>&& outputs,
            std::vector<u64>&& dependencies
        );

        void remove_node(u64 id);

        template<std::ranges::range R>
        void remove_nodes(R&& ids) requires std::same_as<std::ranges::range_value_t<R>, u64> {
            for (u64 const id : ids) {
                this->remove_node(id);
            }
        }

        void prune_node(u64 prune_id);

        [[nodiscard]]
        u64 insert_resource(buffer::Buffer&& resource);

        void remove_resource(u64 id);

        [[nodiscard]]
        std::expected<std::optional<u64>, Error> get_dependency_node(HCDependency const& dependency) const noexcept;

        Bank<Node> nodes;
        std::unordered_set<u64> pruned_node_ids;
        bool dirty = true;

        Bank<buffer::Buffer> resources;
        Bank<buffer::DynamicBuffer> dynamic_resources;
        Bank<texture::Texture> textures;

        struct RenderPassGraphData {
            std::vector<u64> nodes;
            UserPredicate<Sz> predicate;
        };

        RenderPassBank render_passes;
        std::unordered_map<u64, RenderPassGraphData> render_pass_datas;

        u8 graphics_idx = std::numeric_limits<u8>::max(); //!< The index of the graphics command list in `commands`.
        u8 compute_idx = std::numeric_limits<u8>::max(); //!< The index of the compute command list in `commands`.
        u8 transfer_idx = std::numeric_limits<u8>::max(); //!< The index of the transfer command list in `commands`.

        std::vector<std::vector<void*>> commands; //!< A compiled list of commands.
    };
}
