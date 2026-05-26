
#include <pch.hpp>

#include "shader_stages.hpp"

namespace hc::render::pipeline {
    std::expected<ShaderStages, Error> ShaderStages::create(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::vector<Shader> const& shaders
    ) {
        ShaderStages shader_stages;

        shader_stages.shader_stages.reserve(shaders.size());
        shader_stages.shader_modules.reserve(shaders.size());

        for (auto const& shader : shaders) {
            VkShaderModuleCreateInfo module_create_info = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shader.bytecode_vec().size() * sizeof(u32),
                .pCode = shader.bytecode_vec().data(),
            };

            auto& module = shader_stages.shader_modules.back();
            auto result = vk::ShaderModule::create(fn_table, device, module_create_info);
            if (!result) {
                return result.error();
            }
            shader_stages.shader_modules.emplace_back(*std::move(result));

            shader_stages.shader_stages.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = shader.stage_flag(),
                    .module = module,
                    .pName = shader.entrypoint_str(),
                    // TODO specialization constants
                    .pSpecializationInfo = nullptr,
                }
            );
        }

        return shader_stages;
    }

    void ShaderStages::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        for (auto& shader_module : this->shader_modules) {
            fn_table.vkDestroyShaderModule(device, shader_module, nullptr);
            shader_module.destroy(fn_table, device);
        }

        this->shader_modules.clear();
    }

    u32 ShaderStages::stage_count() const noexcept {
        return static_cast<u32>(this->shader_stages.size());
    }

    VkPipelineShaderStageCreateInfo const* ShaderStages::stages() const noexcept {
        return this->shader_stages.data();
    }
}
