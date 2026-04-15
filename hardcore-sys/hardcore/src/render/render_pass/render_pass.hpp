
#pragma once

#include "render_pass_instance_key.hpp"

#include <render/ops/render_pass.h>

#include "../resource/texture.hpp"

#include <core/error.hpp>
#include <util/bank.hpp>

#include <expected>
#include <span>
#include <unordered_map>
#include <vector>

namespace hc::render {
    class RenderPass {
    public:
        [[nodiscard]]
        static std::expected<RenderPass, Error> create(
            std::span<HCSubpass const> const& subpasses,
            Bank<texture::Texture> const& textures
        );

        [[nodiscard]] RenderPassInstanceKey const& instance_key() const noexcept;

    private:
        RenderPass() = default;

        RenderPassInstanceKey key;

        typedef std::pair<u64, HCTextureViewParams> Attachment;

        struct Subpass {
            std::unordered_map<u32, Attachment> inputs;
            std::unordered_map<u32, Attachment> outputs;
            std::optional<Attachment> depth_stencil;
        };

        std::vector<Subpass> subpasses;
    };
}
