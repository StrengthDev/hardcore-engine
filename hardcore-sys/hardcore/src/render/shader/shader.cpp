#include <pch.hpp>

#include "shader.hpp"
#include "util.hpp"

#include <core/log.hpp>
#include <render/shader.h>
#include <render/util.hpp>
#include <util/flow.hpp>
#include <util/static_map.hpp>

namespace hc::render {
    static StaticMap<SparseKey<SpvReflectDescriptorType,
        KeyRange<SpvReflectDescriptorType, SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER, SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT>,
        KeyRange<SpvReflectDescriptorType, SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR>
    >, DescriptorType> constexpr DESCRIPTOR_TYPE_MAP = {
        {SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER, DescriptorType::Sampler},
        {SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DescriptorType::CombinedImageSampler},
        {SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DescriptorType::SampledImage},
        {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE, DescriptorType::StorageImage},
        {SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DescriptorType::UniformTexelBuffer},
        {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DescriptorType::StorageTexelBuffer},
        {SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DescriptorType::UniformBuffer},
        {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER, DescriptorType::StorageBuffer},
        // Why is there a distinction between dynamic and non-dynamic? that is up to the binding code, the shader has no
        // way to know this information
        {SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DescriptorType::UniformBuffer},
        {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DescriptorType::StorageBuffer},
        {SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DescriptorType::InputAttachment},
        {SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, DescriptorType::AccelerationStructure},
    };

    static StaticMap<KeyUnion<BasicKey<u8, 4, 2>, BasicKey<u8, 4, 2>>, HCComposition> constexpr MATRIX_TYPE_MAP = {
        {{2, 2}, HCComposition_Mat2x2},
        {{2, 3}, HCComposition_Mat2x3},
        {{2, 4}, HCComposition_Mat2x4},
        {{3, 2}, HCComposition_Mat3x2},
        {{3, 3}, HCComposition_Mat3x3},
        {{3, 4}, HCComposition_Mat3x4},
        {{4, 2}, HCComposition_Mat4x2},
        {{4, 3}, HCComposition_Mat4x3},
        {{4, 4}, HCComposition_Mat4x4},
    };

    static u32 constexpr PRIMITIVE_TYPE_FLAGS = SPV_REFLECT_TYPE_FLAG_BOOL | SPV_REFLECT_TYPE_FLAG_INT
        | SPV_REFLECT_TYPE_FLAG_FLOAT;

    static u32 constexpr POD_TYPE_FLAGS = SPV_REFLECT_TYPE_FLAG_VECTOR | SPV_REFLECT_TYPE_FLAG_MATRIX
        | PRIMITIVE_TYPE_FLAGS;

    static u32 constexpr OPAQUE_TYPE_FLAGS = SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE
        | SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLER | SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE
        | SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK | SPV_REFLECT_TYPE_FLAG_EXTERNAL_ACCELERATION_STRUCTURE
        | SPV_REFLECT_TYPE_FLAG_EXTERNAL_MASK;

    template<typename ElementType>
    [[nodiscard]]
    static inline PODDescriptor create_array_descriptor(ElementType&& element, SpvReflectBlockVariable const& block_variable) {
        if (block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
            u32 count = 1;
            for (u32 i = 0; i < block_variable.array.dims_count; ++i) {
                count *= block_variable.array.dims[i];
            }

            return ArrayDescriptor<ElementType>{
                .element = std::forward<ElementType>(element),
                .count = count,
                .stride = block_variable.array.stride
            };
        }

        return std::forward<ElementType>(element);
    }

    [[nodiscard]]
    static inline std::expected<PODDescriptor, Error> create_basic_descriptor(SpvReflectBlockVariable const& block_variable) {
        BasicDescriptor descriptor = {};
        descriptor.matrix_stride = std::numeric_limits<decltype(descriptor.matrix_stride)>::max();

        switch (u32 const primitive_flag = block_variable.type_description->type_flags & PRIMITIVE_TYPE_FLAGS) {
        case SPV_REFLECT_TYPE_FLAG_BOOL:
            descriptor.primitive_type = HCPrimitive_Boolean;
            break;
        case SPV_REFLECT_TYPE_FLAG_INT:
            if (block_variable.numeric.scalar.signedness) {
                descriptor.primitive_type = HCPrimitive_Integer;
            } else {
                descriptor.primitive_type = HCPrimitive_Unsigned;
            }
            break;
        case SPV_REFLECT_TYPE_FLAG_FLOAT:
            descriptor.primitive_type = HCPrimitive_Float;
            break;
        default:
            HC_ERROR("Unknown primitive type: " << primitive_flag);
            return Error(HCError_ShaderReflectionFailed);
        }

        descriptor.primitive_size = block_variable.numeric.scalar.width;

        if (block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            u8 const rows = static_cast<u8>(block_variable.numeric.matrix.row_count);
            u8 const columns = static_cast<u8>(block_variable.numeric.matrix.column_count);

            auto matrix_type = MATRIX_TYPE_MAP[{rows, columns}];
            if (!matrix_type) {
                HC_ERROR("Unsupported matrix dimensions: (" << rows << ", " << columns << ')');
                return Error(HCError_ShaderReflectionFailed);
            }

            descriptor.composition = *matrix_type;
            descriptor.matrix_stride = block_variable.numeric.matrix.stride;
        } else if (u32 const count = block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
            switch (block_variable.numeric.vector.component_count) {
            case 2:
                descriptor.composition = HCComposition_Vec2;
                break;
            case 3:
                descriptor.composition = HCComposition_Vec3;
                break;
            case 4:
                descriptor.composition = HCComposition_Vec4;
                break;
            default:
                HC_ERROR("Unsupported vector element count: " << count);
                return Error(HCError_ShaderReflectionFailed);
            }
        } else {
            descriptor.composition = HCComposition_Scalar;
        }

        return create_array_descriptor(std::move(descriptor), block_variable);
    }

    [[nodiscard]]
    static inline std::expected<PODDescriptor, Error> create_buffer_descriptor(
        SpvReflectBlockVariable const& block_variable
    ) {
        if (block_variable.type_description->type_flags & POD_TYPE_FLAGS) {
            return create_basic_descriptor(block_variable);
        } else if (block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) {
            StructDescriptor descriptor;
            descriptor.members.reserve(block_variable.member_count);

            for (u32 i = 0; i < block_variable.member_count; ++i) {
                auto member = create_buffer_descriptor(block_variable.members[i]);
                if (!member) {
                    return member.error();
                }

                descriptor.members.push_back(
                    std::make_unique<MemberDescriptor>(
                        *std::move(member),
                        block_variable.members[i].offset
                    )
                );
            }

            return create_array_descriptor(std::move(descriptor), block_variable);
        }

        if (block_variable.type_description->type_flags & OPAQUE_TYPE_FLAGS) {
            HC_ERROR("Opaque data types are not allowed inside struct types");
        } else {
            HC_ERROR("Unsupported data type: " << block_variable.type_description->type_flags);
        }

        return Error(HCError_ShaderReflectionFailed);
    }

    template<typename T>
    [[nodiscard]]
    static inline std::expected<std::vector<T*>, Error> enumerate(
        const spv_reflect::ShaderModule& module,
        SpvReflectResult (spv_reflect::ShaderModule::*member_fn)(uint32_t*, T**) const,
        const char* name
    ) {
        u32 count = 0;
        auto result = (module.*member_fn)(&count, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to reflect " << name << " count: " << to_str(result));
            return Error(HCError_ShaderReflectionFailed);
        }
        std::vector<T*> vec(count, nullptr);
        result = (module.*member_fn)(&count, vec.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to reflect " << name << " items: " << to_str(result));
            return Error(HCError_ShaderReflectionFailed);
        }

        return vec;
    }

    [[nodiscard]]
    static inline std::expected<DescriptorBinding, Error> create_binding(const SpvReflectDescriptorBinding& reflection) {
        auto descriptor_type_opt = DESCRIPTOR_TYPE_MAP[reflection.descriptor_type];
        if (!descriptor_type_opt) {
            HC_ERROR("Unknown descriptor type: " << reflection.descriptor_type);
            return Error(HCError_ShaderReflectionFailed);
        }

        DescriptorBinding binding;
        binding.name = reflection.name;
        binding.type = *descriptor_type_opt;

        if (binding.type == DescriptorType::UniformBuffer || binding.type == DescriptorType::StorageBuffer) {
            auto result = create_buffer_descriptor(reflection.block);
            if (!result) {
                return result.error();
            }

            binding.descriptor = *std::move(result);
            return binding;
        }

        return binding;
    }

    std::expected<void, Error> Shader::reflect() {
        spv_reflect::ShaderModule const module(this->bytecode, SPV_REFLECT_MODULE_FLAG_NO_COPY);
        if (auto const result = module.GetResult(); result != SPV_REFLECT_RESULT_SUCCESS) {
            HC_ERROR("Failed to run shader reflection: " << to_str(result));
            return Error(HCError_ShaderReflectionFailed);
        }

        auto const bindings_result = enumerate(
            module,
            &spv_reflect::ShaderModule::EnumerateDescriptorBindings,
            "descriptor bindings"
        );
        if (!bindings_result) {
            return Error(HCError_ShaderReflectionFailed);
        }

        for (const auto& binding : *bindings_result) {
            auto binding_result = create_binding(*binding);
            if (!binding_result) {
                return binding_result.error();
            }

            this->bindings.emplace(std::make_pair(binding->set, binding->binding), *std::move(binding_result));
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

