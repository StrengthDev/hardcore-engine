
#include <pch.hpp>

#include "render_pass.hpp"

#include "../renderer.hpp"
#include "../resource/texture.hpp"

#include <util/number.hpp>

namespace hc::render {
    std::expected<RenderPass, Error> RenderPass::create(
        std::span<HCSubpass const> const& subpasses,
        Bank<texture::Texture> const& textures
    ) {
        RenderPass render_pass;

        for (auto const& subpass : subpasses) {
            std::unordered_set<u64> input_textures;
            std::unordered_set<u64> output_textures;

            for (auto const& input : std::span(subpass.inputs, subpass.input_count)) {
                if (!textures.contains(input.texture_id)) {
                    HC_ERROR("Input texture does not exist");
                    return Error(HCError_InvalidParams);
                }

                input_textures.insert(input.texture_id);
            }

            for (auto const& output : std::span(subpass.outputs, subpass.output_count)) {
                if (!textures.contains(output.texture_id)) {
                    HC_ERROR("Output texture does not exist");
                    return Error(HCError_InvalidParams);
                }

                output_textures.insert(output.texture_id);
            }

            std::vector<u64> intersection;
            std::ranges::set_intersection(input_textures, output_textures, std::back_inserter(intersection));
            if (!intersection.empty()) {
                HC_ERROR("The same texture cannot be used for both input and output attachments in the same subpass");
                return Error(HCError_InvalidParams);
            }

            if (subpass.depth_stencil_attachment && !textures.contains(subpass.depth_stencil_attachment->texture_id)) {
                HC_ERROR("Depth stencil texture does not exist");
                return Error(HCError_InvalidParams);
            }
        }

        auto key_result = RenderPassInstanceKey::create(subpasses, textures);
        if (!key_result) {
            return key_result.error();
        }

        render_pass.key = *std::move(key_result);

        render_pass.subpasses.reserve(subpasses.size());

        for (auto const& subpass : subpasses) {
            Subpass subpass_attachments;

            for (auto const& [texture_id, view, index, _] : std::span(subpass.inputs, subpass.input_count)) {
                subpass_attachments.inputs.emplace(index, Attachment{texture_id, view});
            }

            for (auto const& [texture_id, view, location, _] : std::span(subpass.outputs, subpass.output_count)) {
                subpass_attachments.outputs.emplace(location, Attachment{texture_id, view});
            }

            if (subpass.depth_stencil_attachment) {
                subpass_attachments.depth_stencil = Attachment{
                    subpass.depth_stencil_attachment->texture_id,
                    subpass.depth_stencil_attachment->texture_view_params
                };
            }

            render_pass.subpasses.push_back(std::move(subpass_attachments));
        }

        return render_pass;
    }

    RenderPassInstanceKey const& RenderPass::instance_key() const noexcept {
        return this->key;
    }
}
