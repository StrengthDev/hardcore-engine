
#pragma once

#include <render/ops/render_pass.h>

#include "../resource/texture.hpp"
#include "../vulkan.hpp"

#include <core/error.hpp>
#include <util/bank.hpp>

#include <unordered_map>
#include <vector>

namespace hc::render {
    class RenderPassInstanceKey {
    public:
        RenderPassInstanceKey() = default;
        [[nodiscard]]
        static std::expected<RenderPassInstanceKey, Error> create(
            std::span<HCSubpass const> const& subpasses,
            Bank<texture::Texture> const& textures
        );

        [[nodiscard]]
        std::expected<vk::RenderPass, Error> create_instance(
            VolkDeviceTable const& fn_table,
            VkDevice device
        ) const noexcept;

        bool operator==(RenderPassInstanceKey const& other) const noexcept;

        [[nodiscard]] Sz hash() const noexcept;

    private:
        [[nodiscard]]
        std::pair<std::vector<VkAttachmentDescription>, std::vector<VkSubpassDependency>> parse_subpass_attachments() const noexcept;

        std::vector<VkFormat> attachments;

        struct Subpass {
            std::unordered_map<u32, u32> inputs;
            std::unordered_map<u32, u32> outputs;
            std::optional<u32> depth_stencil;
        };

        std::vector<Subpass> subpasses;

        friend struct std::hash<RenderPassInstanceKey>;
    };
}

template<>
struct std::hash<hc::render::RenderPassInstanceKey> {
    std::size_t operator()(hc::render::RenderPassInstanceKey const& key) const noexcept;
};
