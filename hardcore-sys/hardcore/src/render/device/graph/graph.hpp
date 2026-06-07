
#pragma once

#include "node.hpp"

#include "../cleaner.hpp"

#include "../../device/memory/reference.hpp"
#include "../../pipeline/raster_pipeline.hpp"
#include "../../render_pass/render_pass_bank.hpp"
#include "../../resource/buffer.hpp"
#include "../../resource/texture.hpp"
#include "../../shader/shader.hpp"

#include <render/ops/common.h>
#include <render/ops/render_pass.h>

#include <util/bank.hpp>
#include <util/number.hpp>
#include <util/user_predicate.hpp>

#include <expected>
#include <ranges>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace hc::render::device::graph {
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

        void destroy_render_pass(Cleaner& cleaner, u64 id);

        [[nodiscard]]
        std::expected<u64, Error> create_raster_pipeline(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::vector<std::reference_wrapper<Shader const>> const& shaders,
            HCRasterPipelineInfo const& params
        );

        void destroy_raster_pipeline(Cleaner& cleaner, u64 id);

        [[nodiscard]]
        std::expected<u64, Error> create_draw(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkPipelineCache cache,
            u64 render_pass_id,
            u32 subpass,
            u64 pipeline_id,
            u32 vertex_count,
            u32 instance_count
        );

        void destroy_draw(Cleaner& cleaner, u64 id);

        [[nodiscard]]
        std::expected<void, Error> set_draw_push_constants(u64 id, std::span<void const*> const& constant_ptrs);

    private:
        Graph(u8 graphics_idx, u8 compute_idx, u8 transfer_idx, Sz command_queues);

        [[nodiscard]]
        std::expected<u64, Error> insert_node(
            std::vector<ResourceRef>&& inputs,
            std::vector<ResourceRef>&& outputs,
            std::vector<u64>&& dependencies,
            NodeVariant&& variant
        );

        void remove_node(u64 id);

        template<std::ranges::range R>
        void remove_nodes(R&& ids) requires std::same_as<std::ranges::range_value_t<R>, u64> {
            for (u64 const id : ids) {
                this->remove_node(id);
            }
        }

        void prune_node(u64 prune_id);

        template<std::ranges::range R>
        void prune_nodes(R&& ids) requires std::same_as<std::ranges::range_value_t<R>, u64> {
            for (u64 const id : ids) {
                this->prune_node(id);
            }
        }

        [[nodiscard]] u64 insert_resource(buffer::Buffer&& resource);

        void remove_resource(u64 id);

        [[nodiscard]]
        std::expected<std::optional<u64>, Error> get_dependency_node(HCDependency const& dependency) const noexcept;

        Bank<Node> nodes;
        std::unordered_set<u64> pruned_node_ids;
        u64 root_node;
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

        struct RasterPipeline {
            pipeline::RasterPipeline pipeline;
            std::vector<u64> dependents;
        };

        Bank<RasterPipeline> raster_pipelines;
    };
}
