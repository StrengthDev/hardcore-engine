
#pragma once

#include "../shader/shader.hpp"

#include <core/error.hpp>

#include <render/vulkan.hpp>

namespace hc::render::pipeline {
    class ShaderStages {
    public:
        ShaderStages() = default;

        ShaderStages(ShaderStages&&) noexcept = default;
        ShaderStages& operator=(ShaderStages&&) noexcept = default;

        [[nodiscard]]
        static std::expected<ShaderStages, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::vector<Shader> const& shaders
        );

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

        [[nodiscard]] u32 stage_count() const noexcept;
        [[nodiscard]] VkPipelineShaderStageCreateInfo const* stages() const noexcept;

    private:
        std::vector<VkVertexInputBindingDescription> binding_descriptions;
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        std::vector<vk::ShaderModule> shader_modules;
    };
}

