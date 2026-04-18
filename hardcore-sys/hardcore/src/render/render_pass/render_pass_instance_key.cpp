
#include <pch.hpp>

#include "render_pass_instance_key.hpp"

#include "../util.hpp"

#include <util/bits.hpp>

namespace hc::render {
    typedef std::unordered_map<u64, std::unordered_map<HCTextureViewParams, u32>> AttachmentMap;

    static bool try_insert(
        AttachmentMap& attachment_map,
        u64 texture_id,
        HCTextureViewParams const& view_params,
        u32 attachment_count
    ) {
        return attachment_map[texture_id].emplace(view_params, attachment_count).second;
    }

    static bool contains(AttachmentMap& attachment_map, u64 texture_id, HCTextureViewParams const& view_params) {
        return attachment_map.contains(texture_id) && attachment_map[texture_id].contains(view_params);
    }

    std::expected<RenderPassInstanceKey, Error> RenderPassInstanceKey::create(
        std::span<HCSubpass const> const& subpasses,
        Bank<texture::Texture> const& textures
    ) {
        RenderPassInstanceKey key;
        key.subpasses.reserve(subpasses.size());

        AttachmentMap input_map;
        AttachmentMap output_map;
        AttachmentMap depth_stencil_map;

        // TODO maybe do some of these checks on the render pass, and then a preprocessed object gets passed in the arguments
        //  instead of the texture bank

        for (auto const& subpass : subpasses) {
            if (!subpass.inputs && subpass.input_count) {
                HC_ERROR("Subpass input pointer is null, but input count is more than 0");
                return Error(HCError_InvalidParams);
            }

            if (!subpass.outputs || !subpass.output_count) {
                HC_ERROR("Subpass must have at least one output");
                return Error(HCError_InvalidParams);
            }

            std::vector<u32> attachments;
            std::unordered_set<u64> subpass_input_textures;

            // Both inputs and outputs are sorted by index/location respectively to guarantee that equivalent render
            // passes always have matching keys (attachment order could be different)

            std::vector<std::pair<u32, u32>> sorted_inputs;
            sorted_inputs.reserve(subpass.input_count);
            for (u32 i = 0; i < subpass.input_count; ++i) {
                sorted_inputs.emplace_back(subpass.inputs[i].index, i);
            }
            std::ranges::sort(sorted_inputs, {}, &std::pair<u32, u32>::first);

            std::unordered_map<u32, u32> inputs;
            inputs.reserve(subpass.input_count);

            for (u32 i = 0; i < subpass.input_count; ++i) {
                auto const& input = subpass.inputs[sorted_inputs[i].second];

                if (try_insert(input_map, input.texture_id, input.texture_view_params, static_cast<u32>(key.attachments.size()))) {
                    key.attachments.emplace_back(textures[input.texture_id].format());
                }
                u32 const attachment_index = input_map[input.texture_id][input.texture_view_params];

                auto [_, inserted] = inputs.try_emplace(input.index, attachment_index);
                if (!inserted) {
                    HC_ERROR("Subpass input index already in use by another input");
                    return Error(HCError_InvalidParams);
                }

                subpass_input_textures.insert(input.texture_id);

                if (contains(depth_stencil_map, input.texture_id, input.texture_view_params)) {
                    HC_ERROR("The same render pass attachment cannot be used as both an input and a depth stencil buffer");
                    return Error(HCError_InvalidParams);
                }
            }

            std::vector<std::pair<u32, u32>> sorted_outputs;
            sorted_outputs.reserve(subpass.output_count);
            for (u32 i = 0; i < subpass.output_count; ++i) {
                sorted_outputs.emplace_back(subpass.outputs[i].location, i);
            }
            std::ranges::sort(sorted_outputs, {}, &std::pair<u32, u32>::first);

            std::unordered_map<u32, u32> outputs;
            outputs.reserve(subpass.output_count);

            for (u32 i = 0; i < subpass.output_count; ++i) {
                auto const& output = subpass.outputs[sorted_outputs[i].second];

                if (try_insert(output_map, output.texture_id, output.texture_view_params, static_cast<u32>(key.attachments.size()))) {
                    key.attachments.emplace_back(textures[output.texture_id].format());
                }
                u32 const attachment_index = output_map[output.texture_id][output.texture_view_params];

                auto [_, inserted] = outputs.try_emplace(output.location, attachment_index);
                if (!inserted) {
                    HC_ERROR("Subpass output index already in use by another output");
                    return Error(HCError_InvalidParams);
                }

                if (subpass_input_textures.contains(output.texture_id)) {
                    HC_ERROR("A texture cannot be used in input and output attachments at the same time within the same subpass");
                    return Error(HCError_InvalidParams);
                }

                if (contains(depth_stencil_map, output.texture_id, output.texture_view_params)) {
                    HC_ERROR("The same render pass attachment cannot be used as both an output and a depth stencil buffer");
                    return Error(HCError_InvalidParams);
                }
            }

            std::optional<u32> depth_stencil_index;

            if (subpass.depth_stencil_attachment) {
                auto const& depth_stencil = *subpass.depth_stencil_attachment;

                if (try_insert(depth_stencil_map, depth_stencil.texture_id, depth_stencil.texture_view_params, static_cast<u32>(key.attachments.size()))) {
                    key.attachments.emplace_back(textures[depth_stencil.texture_id].format());
                }

                depth_stencil_index = depth_stencil_map[depth_stencil.texture_id][depth_stencil.texture_view_params];

                if (contains(input_map, depth_stencil.texture_id, depth_stencil.texture_view_params)) {
                    HC_ERROR("The same render pass attachment cannot be used as both an input and a depth stencil buffer");
                    return Error(HCError_InvalidParams);
                }

                if (contains(output_map, depth_stencil.texture_id, depth_stencil.texture_view_params)) {
                    HC_ERROR("The same render pass attachment cannot be used as both an output and a depth stencil buffer");
                    return Error(HCError_InvalidParams);
                }
            }

            key.subpasses.emplace_back(
                Subpass{
                    .inputs = std::move(inputs),
                    .outputs = std::move(outputs),
                    .depth_stencil = depth_stencil_index
                }
            );
        }

        return key;
    }

    static std::vector<VkAttachmentReference> attachment_map_to_vector(
        std::unordered_map<u32, u32> const& map,
        VkImageLayout layout
    ) {
        Sz const input_count = map.empty() ? 0 : std::ranges::max(map | std::views::keys) + 1;

        std::vector<VkAttachmentReference> refs = std::vector(
            input_count,
            VkAttachmentReference{
                .attachment = VK_ATTACHMENT_UNUSED,
                .layout = VK_IMAGE_LAYOUT_MAX_ENUM,
            }
        );

        for (auto const& [index, attachment] : map) {
            refs[index] = {
                .attachment = attachment,
                .layout = layout
            };
        }

        return refs;
    }

    std::expected<vk::RenderPass, Error> RenderPassInstanceKey::create_instance(
        VolkDeviceTable const& fn_table,
        VkDevice device
    ) const noexcept {
        struct SubpassAttachmentRefs {
            std::vector<VkAttachmentReference> inputs;
            std::vector<VkAttachmentReference> outputs;
            std::optional<VkAttachmentReference> depth_stencil;
        };

        std::vector<SubpassAttachmentRefs> attachment_refs(this->subpasses.size());

        std::vector<VkSubpassDescription> subpass_descriptions;
        subpass_descriptions.reserve(this->subpasses.size());

        for (auto const& [subpass, refs] : std::views::zip(this->subpasses, attachment_refs)) {
            refs.inputs = attachment_map_to_vector(subpass.inputs, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            refs.outputs = attachment_map_to_vector(subpass.outputs, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            if (subpass.depth_stencil) {
                refs.depth_stencil = {
                    .attachment = *subpass.depth_stencil,
                    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                };
            }

            subpass_descriptions.push_back(
                {
                    .flags = 0,
                    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                    .inputAttachmentCount = static_cast<u32>(refs.inputs.size()),
                    .pInputAttachments = refs.inputs.data(),
                    .colorAttachmentCount = static_cast<u32>(refs.outputs.size()),
                    .pColorAttachments = refs.outputs.data(),
                    .pResolveAttachments = nullptr,
                    .pDepthStencilAttachment = refs.depth_stencil ? &refs.depth_stencil.value() : nullptr,
                    .preserveAttachmentCount = 0,
                    .pPreserveAttachments = nullptr
                }
            );
        }

        auto [attachment_descriptions, dependencies] = this->parse_subpass_attachments();

        VkRenderPassCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .attachmentCount = static_cast<u32>(attachment_descriptions.size()),
            .pAttachments = attachment_descriptions.data(),
            .subpassCount = static_cast<u32>(subpass_descriptions.size()),
            .pSubpasses = subpass_descriptions.data(),
            .dependencyCount = static_cast<u32>(dependencies.size()),
            .pDependencies = dependencies.data(),
        };

        return vk::RenderPass::create(fn_table, device, &create_info);
    }

    static bool equivalent_maps(std::unordered_map<u32, u32> const& lhs, std::unordered_map<u32, u32> const& rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }

        for (auto const& [key, value] : lhs) {
            if (!(rhs.contains(key) && rhs.at(key) == value)) {
                return false;
            }
        }

        return true;
    }

    bool RenderPassInstanceKey::operator==(RenderPassInstanceKey const& other) const noexcept {
        if (this->attachments.size() != other.attachments.size() || this->subpasses.size() != other.subpasses.size()) {
            return false;
        }

        for (auto const& [lhs_attachment, rhs_attachment] : std::views::zip(this->attachments, other.attachments)) {
            if (lhs_attachment != rhs_attachment) {
                return false;
            }
        }

        for (auto const& [lhs_subpass, rhs_subpass] : std::views::zip(this->subpasses, other.subpasses)) {
            if (!equivalent_maps(lhs_subpass.inputs, rhs_subpass.inputs)
                || !equivalent_maps(lhs_subpass.outputs, rhs_subpass.outputs)
                || lhs_subpass.depth_stencil != rhs_subpass.depth_stencil) {
                return false;
            }
        }

        return true;
    }

    static Sz attachment_map_hash(std::unordered_map<u32, u32> const& attachments) {
        Sz combined_hash = 0;

        std::vector<std::pair<u32, u32>> sorted_pairs;
        sorted_pairs.reserve(attachments.size());
        for (auto const& attachment : attachments) {
            sorted_pairs.emplace_back(attachment);
        }
        std::ranges::sort(sorted_pairs, {}, &std::pair<u32, u32>::first);

        for (int i = 0; i < sorted_pairs.size(); ++i) {
            auto const& [index, attachment] = sorted_pairs[i];
            combined_hash ^= std::hash<u64>{}(std::rotl(concat_bits(index, attachment), i));
        }

        return combined_hash;
    }

    // TODO cache this in a mutable variable
    Sz RenderPassInstanceKey::hash() const noexcept {
        Sz combined_hash = 0;

        for (int i = 0; i < this->attachments.size(); ++i) {
            combined_hash ^= std::rotl(std::hash<VkFormat>{}(this->attachments[i]), i);
        }

        for (int i = 0; i < this->subpasses.size(); ++i) {
            auto const& [inputs, outputs, depth_stencil] = this->subpasses[i];

            combined_hash ^= std::rotl(attachment_map_hash(inputs), i);
            combined_hash ^= std::rotl(attachment_map_hash(outputs), i);

            if (depth_stencil) {
                combined_hash ^= std::hash<u32>{}(std::rotl(*depth_stencil, i));
            }
        }

        return combined_hash;
    }

    std::pair<std::vector<VkAttachmentDescription>, std::vector<VkSubpassDependency>> RenderPassInstanceKey::parse_subpass_attachments() const noexcept {
        std::vector<VkAttachmentDescription> attachment_descriptions;
        attachment_descriptions.reserve(this->attachments.size());

        std::vector<VkSubpassDependency> dependencies;

        struct SubpassSets {
            std::unordered_set<u64> input_textures;
            std::unordered_set<u64> output_textures;
        };

        std::vector<SubpassSets> subpass_sets(this->subpasses.size());

        for (auto const& [subpass, sets] : std::views::zip(this->subpasses, subpass_sets)) {
            for (auto const& texture_id : subpass.inputs | std::views::values) {
                sets.input_textures.insert(texture_id);
            }

            for (auto const& texture_id : subpass.outputs | std::views::values) {
                sets.output_textures.insert(texture_id);
            }
        }

        for (Sz i = 0; i < this->attachments.size(); ++i) {
            std::optional<u32> last_depth_stencil_subpass_idx;

            u32 subpass_idx = 0;

            for (auto const& subpass : this->subpasses) {
                if (subpass.depth_stencil && i == *subpass.depth_stencil) {
                    if (last_depth_stencil_subpass_idx) {
                        dependencies.push_back(
                            {
                                .srcSubpass = *last_depth_stencil_subpass_idx,
                                .dstSubpass = subpass_idx,
                                .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                            }
                        );
                    }

                    last_depth_stencil_subpass_idx = subpass_idx;
                }

                subpass_idx++;
            }

            if (!last_depth_stencil_subpass_idx) {
                bool read_before_write = false;
                bool written = false;
                bool last_op_write = false;

                std::optional<u32> last_output_subpass_idx;
                std::vector<u32> input_subpass_idxs;

                subpass_idx = 0;

                for (auto const& [inputs, outputs] : subpass_sets) {
                    if (inputs.contains(i)) {
                        if (!read_before_write && !written) {
                            read_before_write = true;
                        }

                        last_op_write = false;

                        // write -> read dependency
                        if (last_output_subpass_idx) {
                            dependencies.push_back(
                                {
                                    .srcSubpass = *last_output_subpass_idx,
                                    .dstSubpass = subpass_idx,
                                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                    .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                                    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                                }
                            );
                        }

                        input_subpass_idxs.push_back(subpass_idx);
                    }

                    if (outputs.contains(i)) {
                        written = true;
                        last_op_write = true;

                        // write -> write dependency
                        if (input_subpass_idxs.empty() && last_output_subpass_idx) {
                            HC_WARN("Content written by subpass " << subpass_idx << " is not being read");

                            dependencies.push_back(
                                {
                                    .srcSubpass = *last_output_subpass_idx,
                                    .dstSubpass = subpass_idx,
                                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                                }
                            );
                        }

                        // read -> write dependency
                        for (u32 index : input_subpass_idxs) {
                            dependencies.push_back(
                                {
                                    .srcSubpass = index,
                                    .dstSubpass = subpass_idx,
                                    .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                                }
                            );
                        }

                        input_subpass_idxs.clear();
                        last_output_subpass_idx = subpass_idx;
                    }

                    subpass_idx++;
                }

                attachment_descriptions.emplace_back(
                    VkAttachmentDescription{
                        .flags = 0,
                        .format = this->attachments[i],
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = read_before_write ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .storeOp = written ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout = read_before_write ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .finalLayout = last_op_write ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    }
                );
            } else {
                // TODO probably want to give the option to make the buffer "transient" to ignore load/store
                //      wouldnt even need to pass a buffer, just somehow say you need a transient one, and the renderpass
                //      allocates one using lazy allocation

                attachment_descriptions.emplace_back(
                    VkAttachmentDescription{
                        .flags = 0,
                        .format = this->attachments[i],
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    }
                );
            }
        }

        return {attachment_descriptions, dependencies};
    }
}

std::size_t std::hash<hc::render::RenderPassInstanceKey>::operator()(
    hc::render::RenderPassInstanceKey const& key
) const noexcept {
    return key.hash();
}
