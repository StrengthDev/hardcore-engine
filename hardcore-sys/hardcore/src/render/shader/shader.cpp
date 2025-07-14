#include <pch.hpp>

#include "shader.hpp"
#include "util.hpp"

#include <core/log.hpp>
#include <render/shader.h>
#include <render/util.hpp>
#include <util/flow.hpp>

namespace hc::render {
    template<typename T>
    static inline std::expected<std::vector<T*>, Error> enumerate(
        const spv_reflect::ShaderModule& module,
        SpvReflectResult (spv_reflect::ShaderModule::*member_fn)(uint32_t*, T**) const,
        const char* name
    ) {
        u32 count = 0;
        auto res = (module.*member_fn)(&count, nullptr);
        if (res != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to reflect " << name << " count: " << to_str(res));
            return Error(HCError_ShaderReflectionFailed);
        }
        std::vector<T*> vec(count, nullptr);
        res = (module.*member_fn)(&count, vec.data());
        if (res != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to reflect " << name << " items: " << to_str(res));
            return Error(HCError_ShaderReflectionFailed);
        }

        return vec;
    }

    static inline DescriptorBinding create_binding(const SpvReflectDescriptorBinding& reflection) {
        DescriptorType descriptor_type;
        switch (reflection.descriptor_type) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            descriptor_type = DescriptorType::Sampler;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            descriptor_type = DescriptorType::CombinedImageSampler;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            descriptor_type = DescriptorType::SampledImage;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            descriptor_type = DescriptorType::StorageImage;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            descriptor_type = DescriptorType::UniformTexelBuffer;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            descriptor_type = DescriptorType::StorageTexelBuffer;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            descriptor_type = DescriptorType::UniformBuffer;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            descriptor_type = DescriptorType::StorageBuffer;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            descriptor_type = DescriptorType::UniformBufferDynamic;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            descriptor_type = DescriptorType::StorageBufferDynamic;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            descriptor_type = DescriptorType::InputAttachment;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            descriptor_type = DescriptorType::AccelerationStructure;
            break;
        default: HC_UNREACHABLE("All type values must be implemented");
        }

        resource::Descriptor descriptor;
        descriptor.fields.reserve(reflection.type_description->member_count);

        return {.name = reflection.name, .type = descriptor_type, .descriptor = std::move(descriptor),};
    }

    std::expected<void, Error> Shader::reflect() {
        spv_reflect::ShaderModule module(this->bytecode, SPV_REFLECT_MODULE_FLAG_NO_COPY);
        auto result = module.GetResult();
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to run shader reflection: " << to_str(result));
            return Error(HCError_ShaderReflectionFailed);
        }

        auto bindings_result = enumerate(
            module,
            &spv_reflect::ShaderModule::EnumerateDescriptorBindings,
            "descriptor binding"
        );
        if (!bindings_result) {
            return Error(HCError_ShaderReflectionFailed);
        }
        for (const auto& binding : *bindings_result) {
            this->bindings.emplace(std::make_pair(binding->set, binding->binding), create_binding(*binding));
        }

        this->entrypoint = module.GetEntryPointName();

        return {};
    }

    std::expected<Shader, Error> Shader::create(std::vector<u32>&& bytecode, HCShaderStage stage) {
        Shader shader;
        shader.bytecode = std::move(bytecode);
        shader.stage = stage;

        auto result = shader.reflect();
        if (!result) {
            return result.error();
        }

        return shader;
    }

    std::size_t Shader::LocationHash::operator()(const std::pair<u32, u32>& output) const noexcept {
        return static_cast<std::size_t>(output.first) << 32 | output.second;
    }
}

HCResult hc_create_shader(HCShader* shader, const u32* bytecode, size_t size, HCShaderStage stage) {
    if (!shader) {
        HC_ERROR("Null shader pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!bytecode) {
        HC_ERROR("Null bytecode pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    if (!size) {
        HC_ERROR("Invalid bytecode length");
        return {.error = HCError_InvalidParams, .success = false};
    }

    auto shader_result = hc::render::Shader::create(std::vector(bytecode, bytecode + size), stage);
    if (!shader_result) {
        return shader_result.error();
    }

    auto* shader_ptr = new hc::render::Shader;
    *shader_ptr = *std::move(shader_result);
    *shader = {.inner = shader_ptr};

    return {.success = true};
}

void hc_destroy_shader(HCShader* shader) {
    if (shader && shader->inner) {
        delete static_cast<hc::render::Shader*>(shader->inner);
        shader->inner = nullptr;
    }
}

