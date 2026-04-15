
#pragma once

#include "render_pass.hpp"
#include "render_pass_instance_key.hpp"

#include <util/bank.hpp>
#include <util/uncopyable.hpp>

#include <unordered_map>

namespace hc::render {
    class RenderPassBank {
    public:
        RenderPassBank() = default;

        void destroy(VolkDeviceTable const& fn_table, VkDevice device) noexcept;

        std::expected<u64, Error> create_render_pass(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::span<HCSubpass const> const& subpasses,
            Bank<texture::Texture> const& textures
        );

        void destroy_render_pass(VolkDeviceTable const& fn_table, VkDevice device, u64 id) noexcept;

        [[nodiscard]] bool contains(u64 id) const;

        [[nodiscard]] std::pair<RenderPass&, VkRenderPass> operator[](u64 id) noexcept;

        [[nodiscard]] std::pair<RenderPass const&, VkRenderPass> operator[](u64 id) const noexcept;

    private:
        Bank<RenderPass> bank;

        struct RenderPassInstance {
            ExternalHandle<VkRenderPass, VK_NULL_HANDLE> handle;
            u32 ref_count;
        };

        std::unordered_map<RenderPassInstanceKey, RenderPassInstance> instances;
    };
}

