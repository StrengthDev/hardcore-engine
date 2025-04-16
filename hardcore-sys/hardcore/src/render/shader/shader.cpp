#include <pch.hpp>

#include "shader.hpp"
#include "util.hpp"

#include <core/log.hpp>
#include <render/shader.h>
#include <render/util.hpp>
#include <util/flow.hpp>

namespace hc::render {
    template <typename T>
    static inline Result<std::vector<T*>, ShaderResult> enumerate(
        const spv_reflect::ShaderModule& module,
        SpvReflectResult (spv_reflect::ShaderModule::*member_fn)(uint32_t*, T**) const,
        const char* name
    ) {
        u32 count = 0;
        auto res = (module.*member_fn)(&count, nullptr);
        if (res != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to reflect " << name << " count: " << to_str(res));
            return Err(ShaderResult::FailedReflection);
        }
        std::vector<T*> vec(count, nullptr);
        res = (module.*member_fn)(&count, vec.data());
        if (res != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to reflect " << name << " items: " << to_str(res));
            return Err(ShaderResult::FailedReflection);
        }

        return Ok(std::move(vec));
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

    ShaderResult Shader::reflect(Shader& shader) {
        spv_reflect::ShaderModule module(shader.bytecode, SPV_REFLECT_MODULE_FLAG_NO_COPY);
        auto res = module.GetResult();
        if (res != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to run shader reflection: " << to_str(res));
            return ShaderResult::FailedReflection;
        }

        auto bindings_res = enumerate(
            module,
            &spv_reflect::ShaderModule::EnumerateDescriptorBindings,
            "descriptor binding"
        );
        if (!bindings_res) {
            return ShaderResult::FailedReflection;
        }
        for (const auto& binding : bindings_res.ok()) {
            shader.bindings.emplace(std::make_pair(binding->set, binding->binding), create_binding(*binding));
        }

        shader.entrypoint = module.GetEntryPointName();

        return ShaderResult::Success;
    }

    Result<Shader, ShaderResult> Shader::create(std::vector<u32>&& bytecode, HCShaderStage stage) {
        Shader shader;
        shader.bytecode = std::move(bytecode);
        shader.stage = stage;

        auto res = Shader::reflect(shader);
        if (res != ShaderResult::Success) {
            return Err(res);
        }

        return Ok(std::move(shader));
    }

    std::size_t Shader::LocationHash::operator()(const std::pair<u32, u32>& output) const noexcept {
        return static_cast<std::size_t>(output.first) << 32 | output.second;
    }
}

HCShader hc_create_shader(const u32* bytecode, size_t size, HCShaderStage stage) {
    if (!bytecode || !size) {
        return {.inner = nullptr};
    }

    auto shader_res = hc::render::Shader::create(std::vector(bytecode, bytecode + size), stage);
    if (!shader_res) {
        return {.inner = nullptr};
    }

    auto* shader_ptr = new hc::render::Shader;
    *shader_ptr = std::move(shader_res).ok();

    return {.inner = shader_ptr};
}

void hc_destroy_shader(HCShader* shader) {
    if (shader && shader->inner) {
        delete static_cast<hc::render::Shader*>(shader->inner);
        shader->inner = nullptr;
    }
}

