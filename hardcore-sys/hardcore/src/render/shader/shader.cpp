
#include <pch.hpp>

#include "shader.hpp"
#include "util.hpp"

#include <core/log.hpp>
#include <render/util.hpp>

#include <util/bits.hpp>
#include <util/flow.hpp>
#include <util/static_map.hpp>

namespace hc::render {
    static StaticMap<BasicKey<HCShaderStage, HCShaderStage_RayCallable>, VkShaderStageFlagBits> constexpr STAGE_MAP = {
        {HCShaderStage_Vertex, VK_SHADER_STAGE_VERTEX_BIT},
        {HCShaderStage_Fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
        {HCShaderStage_Compute, VK_SHADER_STAGE_COMPUTE_BIT},
        {HCShaderStage_Mesh, VK_SHADER_STAGE_MESH_BIT_EXT},
        {HCShaderStage_TesselationControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
        {HCShaderStage_TesselationEvaluation, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
        {HCShaderStage_Geometry, VK_SHADER_STAGE_GEOMETRY_BIT},
        {HCShaderStage_Task, VK_SHADER_STAGE_TASK_BIT_EXT},
        {HCShaderStage_RayGeneration, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {HCShaderStage_RayIntersection, VK_SHADER_STAGE_INTERSECTION_BIT_KHR},
        {HCShaderStage_RayAnyHit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR},
        {HCShaderStage_RayClosestHit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
        {HCShaderStage_RayMiss, VK_SHADER_STAGE_MISS_BIT_KHR},
        {HCShaderStage_RayCallable, VK_SHADER_STAGE_CALLABLE_BIT_KHR},
    };

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
    static inline std::expected<BasicDescriptor, Error> create_basic_descriptor(
        SpvReflectTypeDescription const& type_description,
        SpvReflectNumericTraits const& numeric_traits
    ) {
        BasicDescriptor descriptor = {};
        descriptor.matrix_stride = std::numeric_limits<decltype(descriptor.matrix_stride)>::max();

        switch (u32 const primitive_flag = type_description.type_flags & PRIMITIVE_TYPE_FLAGS) {
        case SPV_REFLECT_TYPE_FLAG_BOOL:
            descriptor.primitive_type = HCPrimitive_Boolean;
            break;
        case SPV_REFLECT_TYPE_FLAG_INT:
            if (numeric_traits.scalar.signedness) {
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

        descriptor.primitive_size = numeric_traits.scalar.width;

        if (type_description.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            u8 const rows = static_cast<u8>(numeric_traits.matrix.row_count);
            u8 const columns = static_cast<u8>(numeric_traits.matrix.column_count);

            auto matrix_type = MATRIX_TYPE_MAP[{rows, columns}];
            if (!matrix_type) {
                HC_ERROR("Unsupported matrix dimensions: (" << rows << ", " << columns << ')');
                return Error(HCError_ShaderReflectionFailed);
            }

            descriptor.composition = *matrix_type;
            descriptor.matrix_stride = numeric_traits.matrix.stride;
        } else if (u32 const count = type_description.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
            switch (numeric_traits.vector.component_count) {
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

        return descriptor;
    }

    [[nodiscard]]
    static inline std::expected<PODDescriptor, Error> create_basic_pod_descriptor(SpvReflectBlockVariable const& block_variable) {
        auto descriptor_result = create_basic_descriptor(*block_variable.type_description, block_variable.numeric);
        if (!descriptor_result) {
            return descriptor_result.error();
        }

        return create_array_descriptor(*std::move(descriptor_result), block_variable);
    }

    [[nodiscard]]
    static inline std::expected<PODDescriptor, Error> create_buffer_descriptor(
        SpvReflectBlockVariable const& block_variable
    ) {
        if (block_variable.type_description->type_flags & POD_TYPE_FLAGS) {
            return create_basic_pod_descriptor(block_variable);
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

            this->bindings_map.emplace(DescriptorLocation{binding->set, binding->binding}, *std::move(binding_result));
        }

        auto const inputs_result = enumerate(
            module,
            &spv_reflect::ShaderModule::EnumerateInputVariables,
            "input variables"
        );
        if (!inputs_result) {
            return Error(HCError_ShaderReflectionFailed);
        }

        this->inputs.reserve(inputs_result.value().size());
        for (auto const& input : *inputs_result) {
            auto descriptor_result = create_basic_descriptor(*input->type_description, input->numeric);
            if (!descriptor_result) {
                return descriptor_result.error();
            }

            inputs.push_back(
                {
                    .descriptor = *std::move(descriptor_result),
                    .location = input->location
                }
            );
        }

        this->entrypoint = module.GetEntryPointName();

        return {};
    }

    std::expected<Shader, Error> Shader::create(std::vector<u32>&& bytecode, HCShaderStage stage) {
        Shader shader;
        shader.bytecode = std::move(bytecode);
        shader.stage = stage;

        if (auto result = shader.reflect(); !result) {
            return result.error();
        }

        return shader;
    }

    VkShaderStageFlagBits Shader::stage_flag() const noexcept {
        auto const* stage_flag = STAGE_MAP[this->stage];
        HC_ASSERT(stage_flag, "All shader stages must have a matching flag");

        return *stage_flag;
    }

    char const* Shader::entrypoint_str() const noexcept {
        return this->entrypoint.c_str();
    }

    std::unordered_map<DescriptorLocation, DescriptorBinding> const& Shader::bindings() const noexcept {
        return this->bindings_map;
    }

    std::vector<ShaderInput> const& Shader::inputs_vec() const noexcept {
        return this->inputs;
    }

    std::vector<u32> const& Shader::bytecode_vec() const noexcept {
        return this->bytecode;
    }

    bool DescriptorLocation::operator==(DescriptorLocation const& other) const noexcept {
        return this->set == other.set && this->binding == other.binding;
    }
}

std::size_t std::hash<hc::render::DescriptorLocation>::operator()(
    const hc::render::DescriptorLocation& location
) const noexcept {
    return std::hash<u64>{}(concat_bits(location.set, location.binding));
}
