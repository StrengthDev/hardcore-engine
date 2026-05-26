
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
        { HCShaderStage_Vertex, VK_SHADER_STAGE_VERTEX_BIT },
        { HCShaderStage_Fragment, VK_SHADER_STAGE_FRAGMENT_BIT },
        { HCShaderStage_Compute, VK_SHADER_STAGE_COMPUTE_BIT },
        { HCShaderStage_Mesh, VK_SHADER_STAGE_MESH_BIT_EXT },
        { HCShaderStage_TesselationControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT },
        { HCShaderStage_TesselationEvaluation, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT },
        { HCShaderStage_Geometry, VK_SHADER_STAGE_GEOMETRY_BIT },
        { HCShaderStage_Task, VK_SHADER_STAGE_TASK_BIT_EXT },
        { HCShaderStage_RayGeneration, VK_SHADER_STAGE_RAYGEN_BIT_KHR },
        { HCShaderStage_RayIntersection, VK_SHADER_STAGE_INTERSECTION_BIT_KHR },
        { HCShaderStage_RayAnyHit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR },
        { HCShaderStage_RayClosestHit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR },
        { HCShaderStage_RayMiss, VK_SHADER_STAGE_MISS_BIT_KHR },
        { HCShaderStage_RayCallable, VK_SHADER_STAGE_CALLABLE_BIT_KHR },
    };

    static StaticMap<SparseKey<SpvReflectDescriptorType,
        KeyRange<SpvReflectDescriptorType, SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER, SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT>,
        KeyRange<SpvReflectDescriptorType, SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR>
    >, DescriptorType> constexpr DESCRIPTOR_TYPE_MAP = {
        { SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER, DescriptorType::Sampler },
        { SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DescriptorType::CombinedImageSampler },
        { SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DescriptorType::SampledImage },
        { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE, DescriptorType::StorageImage },
        { SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DescriptorType::UniformTexelBuffer },
        { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DescriptorType::StorageTexelBuffer },
        { SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DescriptorType::UniformBuffer },
        { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER, DescriptorType::StorageBuffer },
        // Why is there a distinction between dynamic and non-dynamic? that is up to the binding code, the shader has no
        // way to know this information
        { SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DescriptorType::UniformBuffer },
        { SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DescriptorType::StorageBuffer },
        { SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DescriptorType::InputAttachment },
        { SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, DescriptorType::AccelerationStructure },
    };

    static StaticMap<KeyUnion<BasicKey<u8, 4, 2>, BasicKey<u8, 4, 2>>, HCComposition> constexpr MATRIX_TYPE_MAP = {
        { { 2, 2 }, HCComposition_Mat2x2 },
        { { 2, 3 }, HCComposition_Mat2x3 },
        { { 2, 4 }, HCComposition_Mat2x4 },
        { { 3, 2 }, HCComposition_Mat3x2 },
        { { 3, 3 }, HCComposition_Mat3x3 },
        { { 3, 4 }, HCComposition_Mat3x4 },
        { { 4, 2 }, HCComposition_Mat4x2 },
        { { 4, 3 }, HCComposition_Mat4x3 },
        { { 4, 4 }, HCComposition_Mat4x4 },
    };

    static u32 constexpr PRIMITIVE_TYPE_FLAGS = SPV_REFLECT_TYPE_FLAG_BOOL | SPV_REFLECT_TYPE_FLAG_INT
        | SPV_REFLECT_TYPE_FLAG_FLOAT;

    static u32 constexpr POD_TYPE_FLAGS = SPV_REFLECT_TYPE_FLAG_VECTOR | SPV_REFLECT_TYPE_FLAG_MATRIX
        | PRIMITIVE_TYPE_FLAGS;

    static u32 constexpr OPAQUE_TYPE_FLAGS = SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE
        | SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLER | SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE
        | SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK | SPV_REFLECT_TYPE_FLAG_EXTERNAL_ACCELERATION_STRUCTURE
        | SPV_REFLECT_TYPE_FLAG_EXTERNAL_MASK;

    template<typename T>
    concept ReflectedVariable = requires(T value) {
        { value.member_count } -> std::convertible_to<u32>;
        { value.numeric } -> std::convertible_to<SpvReflectNumericTraits>;
        { value.members } -> std::convertible_to<T*>;
        { value.type_description } -> std::convertible_to<SpvReflectTypeDescription*>;
    };

    [[nodiscard]]
    static inline std::expected<Sz, Error> parse_basic_descriptor(
        SpvReflectTypeDescription const& type_description,
        SpvReflectNumericTraits const& numeric_traits,
        std::vector<HCTypeDescriptor>& descriptor_data
    ) {
        HCBasicDescriptor descriptor = {};
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

        Sz size = descriptor.primitive_type;

        if (type_description.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            u8 const rows = static_cast<u8>(numeric_traits.matrix.row_count);
            u8 const columns = static_cast<u8>(numeric_traits.matrix.column_count);

            auto matrix_type = MATRIX_TYPE_MAP[{ rows, columns }];
            if (!matrix_type) {
                HC_ERROR("Unsupported matrix dimensions: (" << rows << ", " << columns << ')');
                return Error(HCError_ShaderReflectionFailed);
            }

            descriptor.composition = *matrix_type;
            descriptor.matrix_stride = numeric_traits.matrix.stride;

            size = descriptor.matrix_stride * columns;
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

            size *= numeric_traits.vector.component_count;
            // Convert bits to bytes
            size = size / 8 + (size % 8 ? 1 : 0);
        } else {
            descriptor.composition = HCComposition_Scalar;
            // Convert bits to bytes
            size = size / 8 + (size % 8 ? 1 : 0);
        }

        descriptor_data.emplace_back(
            HCTypeDescriptor {
                .type_category = HCDescriptorCategory_Basic,
                .type_descriptor = {
                    .basic_descriptor = descriptor,
                },
            }
        );

        return size;
    }

    template<ReflectedVariable V>
    [[nodiscard]]
    static inline std::expected<Sz, Error> parse_descriptor(V const&, std::vector<HCTypeDescriptor>&);

    [[nodiscard]]
    static inline std::expected<Sz, Error> parse_struct_descriptor(
        SpvReflectBlockVariable const& block_variable,
        std::vector<HCTypeDescriptor>& descriptor_data
    ) {
        u16 const first_member_idx = static_cast<u16>(descriptor_data.size()) + 1;

        descriptor_data.emplace_back(
            HCTypeDescriptor {
                .type_category = HCDescriptorCategory_Struct,
                .type_descriptor = {
                    .struct_descriptor = {
                        .first_member_type_idx = first_member_idx,
                        .member_count = static_cast<u16>(block_variable.member_count)
                    },
                },
            }
        );

        std::vector<u16> member_indexes;
        member_indexes.reserve(block_variable.member_count);

        for (u32 i = 0; i < block_variable.member_count; ++i) {
            descriptor_data.emplace_back(
                HCTypeDescriptor {
                    .type_category = HCDescriptorCategory_Member,
                    .type_descriptor = {
                        .member_descriptor = {
                            .type_idx = 0,
                            .offset = block_variable.members[i].offset,
                        },
                    },
                }
            );

            member_indexes.push_back(static_cast<u16>(first_member_idx + i));
        }

        Sz size = 0;

        for (u32 i = 0; i < block_variable.member_count; ++i) {
            descriptor_data[member_indexes[i]].type_descriptor.member_descriptor.type_idx = static_cast<u16>(descriptor_data.size());

            auto result = parse_descriptor(block_variable.members[i], descriptor_data);
            if (!result) {
                return result.error();
            }

            size += *result;
        }

        return size;
    }

    template<ReflectedVariable V>
    [[nodiscard]]
    static inline std::expected<Sz, Error> parse_descriptor(
        V const& block_variable,
        std::vector<HCTypeDescriptor>& descriptor_data
    ) {
        if (block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_REF) {
            u16 const type_idx = static_cast<u16>(descriptor_data.size()) + 1;

            descriptor_data.push_back(
                HCTypeDescriptor {
                    .type_category = HCDescriptorCategory_Pointer,
                    .type_descriptor = {
                        .pointer_descriptor = {
                            .type_idx = type_idx,
                        },
                    },
                }
            );
        }

        u32 count = 1;

        if (block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
            for (u32 i = 0; i < block_variable.type_description->traits.array.dims_count; ++i) {
                count *= block_variable.type_description->traits.array.dims[i];
            }

            u16 const type_idx = static_cast<u16>(descriptor_data.size()) + 1;

            descriptor_data.push_back(
                HCTypeDescriptor {
                    .type_category = HCDescriptorCategory_Array,
                    .type_descriptor = {
                        .array_descriptor = {
                            .element_type_idx = type_idx,
                            .count = count,
                            .stride = block_variable.type_description->traits.array.stride,
                        },
                    },
                }
            );
        }

        std::expected<Sz, Error> result = Error(HCError_ShaderReflectionFailed);

        if (block_variable.type_description->type_flags & POD_TYPE_FLAGS) {
            result = parse_basic_descriptor(
                *block_variable.type_description,
                block_variable.numeric,
                descriptor_data
            );
        } else if (block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) {
            if constexpr (std::is_same_v<V, SpvReflectBlockVariable>) {
                result = parse_struct_descriptor(
                    block_variable,
                    descriptor_data
                );
            } else {
                HC_ERROR("Input and output variables may not be struct types");
            }
        } else if (block_variable.type_description->type_flags & OPAQUE_TYPE_FLAGS) {
            HC_ERROR("Opaque data types are not allowed inside struct types");
        } else {
            HC_ERROR("Unsupported data type: " << block_variable.type_description->type_flags);
        }

        if (!result) {
            return result.error();
        }

        Sz size;
        if (block_variable.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_REF) {
            size = 8;
        } else {
            size = count * *result;
        }

        return size;
    }

    template<ReflectedVariable V>
    [[nodiscard]]
    static inline std::expected<std::pair<std::vector<HCTypeDescriptor>, Sz>, Error> create_descriptor(
        V const& block_variable
    ) {
        std::vector<HCTypeDescriptor> descriptor_data;

        auto result = parse_descriptor(block_variable, descriptor_data);
        if (!result) {
            return result.error();
        }

        return std::pair(std::move(descriptor_data), *result);
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
    static inline std::expected<DescriptorBinding, Error> create_binding(
        const SpvReflectDescriptorBinding& reflection,
        DescriptorBinding (*builder)(
            std::string&& name,
            DescriptorType type,
            std::vector<HCTypeDescriptor>&& descriptor_data,
            Sz total_size
        )
    ) {
        auto const descriptor_type_opt = DESCRIPTOR_TYPE_MAP[reflection.descriptor_type];
        if (!descriptor_type_opt) {
            HC_ERROR("Unknown descriptor type: " << reflection.descriptor_type);
            return Error(HCError_ShaderReflectionFailed);
        }
        auto const descriptor_type = *descriptor_type_opt;

        if (descriptor_type == DescriptorType::UniformBuffer || descriptor_type == DescriptorType::StorageBuffer) {
            auto result = create_descriptor(reflection.block);
            if (!result) {
                return result.error();
            }

            return builder(reflection.name, descriptor_type, std::move(result->first), result->second);
        }

        return builder(reflection.name, descriptor_type, {}, 0);
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
            auto binding_result = create_binding(*binding, Shader::create_binding_struct);
            if (!binding_result) {
                return binding_result.error();
            }

            this->bindings_map.emplace(DescriptorLocation { binding->set, binding->binding }, *std::move(binding_result));
        }

        auto const inputs_result = enumerate(
            module,
            &spv_reflect::ShaderModule::EnumerateInputVariables,
            "input variables"
        );
        if (!inputs_result) {
            return Error(HCError_ShaderReflectionFailed);
        }

        this->input_vec.reserve(inputs_result.value().size());
        for (auto const& input : *inputs_result) {
            auto descriptor_result = create_descriptor(*input);
            if (!descriptor_result) {
                return descriptor_result.error();
            }

            this->input_vec.push_back(
                {
                    .descriptor = *std::move(descriptor_result),
                    .location = input->location
                }
            );
        }

        auto const outputs_result = enumerate(
            module,
            &spv_reflect::ShaderModule::EnumerateOutputVariables,
            "output variables"
        );
        if (!outputs_result) {
            return Error(HCError_ShaderReflectionFailed);
        }

        for (auto const& output : *outputs_result) {
            if (output->location == std::numeric_limits<decltype(output->location)>::max()) {
                continue;
            }

            auto descriptor_result = create_descriptor(*output);
            if (!descriptor_result) {
                return descriptor_result.error();
            }

            this->output_map.emplace(output->location, Descriptor2(*std::move(descriptor_result)));
        }

        auto const push_constants_result = enumerate(
            module,
            &spv_reflect::ShaderModule::EnumeratePushConstantBlocks,
            "push constants"
        );
        if (!push_constants_result) {
            return Error(HCError_ShaderReflectionFailed);
        }

        if (!push_constants_result.value().empty()) {
            auto descriptor_result = create_descriptor(*push_constants_result.value().front());
            if (!descriptor_result) {
                return descriptor_result.error();
            }

            PushConstant push_constant = {
                .descriptor = *std::move(descriptor_result),
                // TODO offset
                .offset = 0,
            };

            this->push_constants_descriptor = std::move(push_constant);
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

    std::vector<ShaderInput> const& Shader::inputs() const noexcept {
        return this->input_vec;
    }

    std::unordered_map<u32, Descriptor2> const& Shader::outputs() const noexcept {
        return this->output_map;
    }

    std::optional<PushConstant> const& Shader::push_constants() const noexcept {
        return this->push_constants_descriptor;
    }

    std::vector<u32> const& Shader::bytecode_vec() const noexcept {
        return this->bytecode;
    }

    DescriptorBinding Shader::create_binding_struct(
        std::string&& name,
        DescriptorType type,
        std::vector<HCTypeDescriptor>&& descriptor_data,
        Sz total_size
    ) {
        return {
            .name = std::move(name),
            .type = type,
            .descriptor = Descriptor2(std::move(descriptor_data), total_size),
        };
    }

    bool DescriptorLocation::operator==(DescriptorLocation const& other) const noexcept {
        return this->set == other.set && this->binding == other.binding;
    }
}

std::size_t std::hash<hc::render::DescriptorLocation>::operator()(
    const hc::render::DescriptorLocation& location
) const noexcept {
    return std::hash<u64> {}(concat_bits(location.set, location.binding));
}
