#include <pch.hpp>

#include "raster_pipeline.hpp"

#include "../renderer.hpp"
#include "../util.hpp"

#include <render/ops/raster_pipeline.h>

#include <util/static_map.hpp>

namespace hc::render {
    static StaticMap<KeyUnion<
        SparseKey<u8, KeyRange<u8, 1, 2>, KeyRange<u8, 4, 4>, KeyRange<u8, 8, 8>>,
        BasicKey<VertexNumericFormat, VertexNumericFormat::SFloat>,
        BasicKey<HCComposition, HCComposition_Vec4>
    >, VkFormat> constexpr ATTRIBUTE_FORMAT_MAP = {
        {{1, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R8_UNORM},
        {{2, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R16_UNORM},
        // {{4, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R32_UNORM},
        // {{8, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R64_UNORM},
        {{1, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R8_UNORM},
        {{2, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R16_UNORM},
        // {{4, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R32_UNORM},
        // {{8, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R64_UNORM},
        {{1, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R8_USCALED},
        {{2, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R16_USCALED},
        // {{4, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R32_USCALED},
        // {{8, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R64_USCALED},
        {{1, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R8_SSCALED},
        {{2, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R16_SSCALED},
        // {{4, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R32_SSCALED},
        // {{8, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R64_SSCALED},
        {{1, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R8_UINT},
        {{2, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R16_UINT},
        {{4, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R32_UINT},
        {{8, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R64_UINT},
        {{1, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R8_SINT},
        {{2, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R16_SINT},
        {{4, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R32_SINT},
        {{8, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R64_SINT},
        // {{1, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R8_UFLOAT},
        // {{2, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R16_UFLOAT},
        // {{4, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R32_UFLOAT},
        // {{8, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R64_UFLOAT},
        // {{1, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R8_SFLOAT},
        {{2, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R16_SFLOAT},
        {{4, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R32_SFLOAT},
        {{8, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R64_SFLOAT},

        {{1, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R8G8_UNORM},
        {{2, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R16G16_UNORM},
        // {{4, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R32G32_UNORM},
        // {{8, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R64G64_UNORM},
        {{1, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R8G8_UNORM},
        {{2, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R16G16_UNORM},
        // {{4, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R32G32_UNORM},
        // {{8, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R64G64_UNORM},
        {{1, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R8G8_USCALED},
        {{2, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R16G16_USCALED},
        // {{4, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R32G32_USCALED},
        // {{8, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R64G64_USCALED},
        {{1, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R8G8_SSCALED},
        {{2, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R16G16_SSCALED},
        // {{4, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R32G32_SSCALED},
        // {{8, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R64G64_SSCALED},
        {{1, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R8G8_UINT},
        {{2, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R16G16_UINT},
        {{4, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R32G32_UINT},
        {{8, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R64G64_UINT},
        {{1, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R8G8_SINT},
        {{2, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R16G16_SINT},
        {{4, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R32G32_SINT},
        {{8, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R64G64_SINT},
        // {{1, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R8G8_UFLOAT},
        // {{2, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R16G16_UFLOAT},
        // {{4, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R32G32_UFLOAT},
        // {{8, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R64G64_UFLOAT},
        // {{1, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R8G8_SFLOAT},
        {{2, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R16G16_SFLOAT},
        {{4, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R32G32_SFLOAT},
        {{8, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R64G64_SFLOAT},

        {{1, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R8G8B8_UNORM},
        {{2, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R16G16B16_UNORM},
        // {{4, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R32G32B32_UNORM},
        // {{8, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R64G64B64_UNORM},
        {{1, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R8G8B8_UNORM},
        {{2, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R16G16B16_UNORM},
        // {{4, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R32G32B32_UNORM},
        // {{8, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R64G64B64_UNORM},
        {{1, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R8G8B8_USCALED},
        {{2, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R16G16B16_USCALED},
        // {{4, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R32G32B32_USCALED},
        // {{8, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R64G64B64_USCALED},
        {{1, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R8G8B8_SSCALED},
        {{2, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R16G16B16_SSCALED},
        // {{4, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R32G32B32_SSCALED},
        // {{8, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R64G64B64_SSCALED},
        {{1, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R8G8B8_UINT},
        {{2, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R16G16B16_UINT},
        {{4, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R32G32B32_UINT},
        {{8, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R64G64B64_UINT},
        {{1, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R8G8B8_SINT},
        {{2, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R16G16B16_SINT},
        {{4, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R32G32B32_SINT},
        {{8, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R64G64B64_SINT},
        // {{1, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R8G8B8_UFLOAT},
        // {{2, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R16G16B16_UFLOAT},
        // {{4, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R32G32B32_UFLOAT},
        // {{8, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R64G64B64_UFLOAT},
        // {{1, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R8G8B8_SFLOAT},
        {{2, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R16G16B16_SFLOAT},
        {{4, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R32G32B32_SFLOAT},
        {{8, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R64G64B64_SFLOAT},

        {{1, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_UNORM},
        {{2, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_UNORM},
        // {{4, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_UNORM},
        // {{8, VertexNumericFormat::UNorm, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_UNORM},
        {{1, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_UNORM},
        {{2, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_UNORM},
        // {{4, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_UNORM},
        // {{8, VertexNumericFormat::SNorm, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_UNORM},
        {{1, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_USCALED},
        {{2, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_USCALED},
        // {{4, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_USCALED},
        // {{8, VertexNumericFormat::UScaled, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_USCALED},
        {{1, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_SSCALED},
        {{2, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_SSCALED},
        // {{4, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_SSCALED},
        // {{8, VertexNumericFormat::SScaled, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_SSCALED},
        {{1, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_UINT},
        {{2, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_UINT},
        {{4, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_UINT},
        {{8, VertexNumericFormat::UInt, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_UINT},
        {{1, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_SINT},
        {{2, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_SINT},
        {{4, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_SINT},
        {{8, VertexNumericFormat::SInt, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_SINT},
        // {{1, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_UFLOAT},
        // {{2, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_UFLOAT},
        // {{4, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_UFLOAT},
        // {{8, VertexNumericFormat::UFloat, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_UFLOAT},
        // {{1, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R8G8B8A8_SFLOAT},
        {{2, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R16G16B16A16_SFLOAT},
        {{4, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R32G32B32A32_SFLOAT},
        {{8, VertexNumericFormat::SFloat, HCComposition_Scalar}, VK_FORMAT_R64G64B64A64_SFLOAT},
    };

    static inline std::expected<std::tuple<std::vector<VkVertexInputBindingDescription>, std::vector<VkVertexInputAttributeDescription>>, Error> create_input_state_descriptions(
        std::vector<InputBufferDescription> const& buffer_descriptions,
        std::vector<ShaderInput> const& shader_inputs
    ) {
        struct Attribute {
            u32 binding;
            u8 size;
            VertexNumericFormat format;
            HCComposition composition;
            u32 offset;
        };

        std::vector<VkVertexInputBindingDescription> binding_descriptions;
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        std::unordered_map<u32, Attribute> attribute_map;

        binding_descriptions.reserve(buffer_descriptions.size());
        for (auto const& buffer : buffer_descriptions) {
            binding_descriptions.push_back(
                {
                    .binding = buffer.binding,
                    .stride = buffer.stride,
                    .inputRate = buffer.input_rate
                }
            );

            for (auto const& attribute : buffer.attributes) {
                if (attribute_map.contains(attribute.location)) {
                    HC_ERROR("Input buffers have conflicting duplicate attributes");
                    return Error(HCError_InvalidParams);
                }

                attribute_map.emplace(
                    attribute.location,
                    Attribute{
                        .binding = buffer.binding,
                        .size = attribute.size,
                        .format = attribute.format,
                        .composition = attribute.composition,
                        .offset = attribute.offset,
                    }
                );
            }
        }

        attribute_descriptions.reserve(shader_inputs.size());
        for (auto const& input : shader_inputs) {
            if (!attribute_map.contains(input.location)) {
                HC_ERROR("Attribute not bound to any buffer binding: " << input.location);
                return Error(HCError_InvalidParams);
            }

            auto const& [binding, size, format, composition, offset] = attribute_map[input.location];

            bool const format_interpreted_as_float = !(format == VertexNumericFormat::UInt || format == VertexNumericFormat::SInt);
            bool const input_used_as_float = input.descriptor.primitive_type == HCPrimitive_Float;
            if ((format_interpreted_as_float && !input_used_as_float) || (!format_interpreted_as_float && input_used_as_float)) {
                HC_ERROR(
                    "Buffer attribute format does not match shader input variable type: binding="
                    << binding << " location=" << input.location
                );
                return Error(HCError_InvalidParams);
            }

            VkFormat const* attribute_format = ATTRIBUTE_FORMAT_MAP[{size, format, composition}];
            if (!attribute_format) {
                HC_ERROR("Unsupported attribute format");
                return Error(HCError_InvalidParams);
            }

            attribute_descriptions.push_back(
                VkVertexInputAttributeDescription{
                    .location = input.location,
                    .binding = binding,
                    .format = *attribute_format,
                    .offset = offset,
                }
            );
        }

        return std::pair(std::move(binding_descriptions), std::move(attribute_descriptions));
    }

    class RasterPipelineShaders {
    public:
        [[nodiscard]]
        static std::expected<RasterPipelineShaders, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::vector<Shader> const& shaders,
            std::vector<InputBufferDescription> const& buffer_descriptions
        );

        ~RasterPipelineShaders();

        RasterPipelineShaders(RasterPipelineShaders&& other) = default;

        [[nodiscard]] u32 stage_count() const noexcept;
        [[nodiscard]] VkPipelineShaderStageCreateInfo const* stages() const noexcept;
        [[nodiscard]] VkPipelineVertexInputStateCreateInfo const* input_state() const noexcept;

    private:
        RasterPipelineShaders() = default;

        void destroy();

        VkDevice device = VK_NULL_HANDLE;
        // This class is created and destroyed during pipeline creation and nowhere else, so a pointer to the device's
        // function table can be safely stored
        VolkDeviceTable const* fn_table = nullptr;

        VkPipelineVertexInputStateCreateInfo vertex_input_state = {};

        std::vector<VkVertexInputBindingDescription> binding_descriptions;
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        std::vector<ExternalHandle<VkShaderModule, VK_NULL_HANDLE>> shader_modules;
    };

    std::expected<RasterPipelineShaders, Error> RasterPipelineShaders::create(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::vector<Shader> const& shaders,
        std::vector<InputBufferDescription> const& buffer_descriptions
    ) {
        RasterPipelineShaders pipeline_shaders;

        pipeline_shaders.device = device;
        pipeline_shaders.fn_table = &fn_table;

        pipeline_shaders.vertex_input_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        };

        pipeline_shaders.shader_stages.reserve(shaders.size());
        pipeline_shaders.shader_modules.reserve(shaders.size());

        for (auto const& shader : shaders) {
            VkShaderModuleCreateInfo module_create_info = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shader.bytecode_vec().size(),
                .pCode = shader.bytecode_vec().data(),
            };

            pipeline_shaders.shader_modules.emplace_back(VK_NULL_HANDLE);
            auto& module = pipeline_shaders.shader_modules.back();
            VkResult result = fn_table.vkCreateShaderModule(device, &module_create_info, nullptr, &module.get());
            if (result != VK_SUCCESS) {
                pipeline_shaders.shader_modules.pop_back();
                pipeline_shaders.destroy();

                HC_ERROR("Failed to create shader module: " << to_str(result));
                return Error(result);
            }

            pipeline_shaders.shader_stages.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = shader.stage_flag(),
                    .module = module,
                    .pName = shader.entrypoint_str(),
                    .pSpecializationInfo = nullptr,
                }
            );

            if (shader.stage_flag() == VK_SHADER_STAGE_VERTEX_BIT) {
                auto input_state_result = create_input_state_descriptions(buffer_descriptions, shader.inputs_vec());
                if (!input_state_result) {
                    pipeline_shaders.destroy();

                    return input_state_result.error();
                }

                std::tie(pipeline_shaders.binding_descriptions, pipeline_shaders.attribute_descriptions) = *std::move(input_state_result);

                pipeline_shaders.vertex_input_state.vertexBindingDescriptionCount = static_cast<u32>(pipeline_shaders.binding_descriptions.size());
                pipeline_shaders.vertex_input_state.pVertexBindingDescriptions = pipeline_shaders.binding_descriptions.data();
                pipeline_shaders.vertex_input_state.vertexAttributeDescriptionCount = static_cast<u32>(pipeline_shaders.attribute_descriptions.size());
                pipeline_shaders.vertex_input_state.pVertexAttributeDescriptions = pipeline_shaders.attribute_descriptions.data();
            }
        }

        return pipeline_shaders;
    }

    RasterPipelineShaders::~RasterPipelineShaders() {
        this->destroy();
    }

    u32 RasterPipelineShaders::stage_count() const noexcept {
        return static_cast<u32>(this->shader_stages.size());
    }

    VkPipelineShaderStageCreateInfo const* RasterPipelineShaders::stages() const noexcept {
        return this->shader_stages.data();
    }

    VkPipelineVertexInputStateCreateInfo const* RasterPipelineShaders::input_state() const noexcept {
        return &this->vertex_input_state;
    }

    void RasterPipelineShaders::destroy() {
        for (auto& shader_module : this->shader_modules) {
            this->fn_table->vkDestroyShaderModule(device, shader_module, nullptr);
            shader_module.destroy();
        }

        this->shader_modules.clear();
    }

    std::expected<RasterPipeline, Error> RasterPipeline::create(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkPipelineCache cache,
        std::vector<Shader> const& shaders,
        std::vector<InputBufferDescription> const& buffer_descriptions
    ) {
        RasterPipeline pipeline;

        auto descriptor_pool_result = DescriptorPool::create(fn_table, device, shaders);
        if (!descriptor_pool_result) {
            return descriptor_pool_result.error();
        }
        pipeline.descriptor_pool = *std::move(descriptor_pool_result);

        auto pipeline_shaders_result = RasterPipelineShaders::create(fn_table, device, shaders, buffer_descriptions);
        if (!pipeline_shaders_result) {
            return pipeline_shaders_result.error();
        }
        RasterPipelineShaders pipeline_shaders = *std::move(pipeline_shaders_result);

        VkPipelineInputAssemblyStateCreateInfo input_assembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_TRUE,
        };

        VkGraphicsPipelineCreateInfo pipeline_info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = pipeline_shaders.stage_count(),
            .pStages = pipeline_shaders.stages(),
            .pVertexInputState = pipeline_shaders.input_state(),
            .pInputAssemblyState = &input_assembly,
            // .pTessellationState = ,
            // .pViewportState = ,
            // .pRasterizationState = ,
            // .pMultisampleState = ,
            // .pDepthStencilState = ,
            // .pColorBlendState = ,
            // .pDynamicState = ,
            // .layout = ,
            // .renderPass = ,
            // .subpass = ,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };

        VkResult result = fn_table.vkCreateGraphicsPipelines(device, cache, 1, &pipeline_info, nullptr, &pipeline.handle.get());
        if (result != VK_SUCCESS) {
            pipeline.destroy(fn_table, device);

            HC_ERROR("Failed to create pipeline: " << to_str(result));
            return Error(result);
        }

        return pipeline;
    }

    void RasterPipeline::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        if (this->handle.valid()) {
            fn_table.vkDestroyPipeline(device, this->handle, nullptr);
        }

        this->descriptor_pool.destroy(fn_table, device);
    }
}

HCResult hc_new_raster_pipeline(
    HCRasterPipeline* raster_pipeline,
    uint32_t device,
    HCOperationPredicate,
    //predicate,
    void* //user_data


) {
    if (!raster_pipeline) {
        HC_ERROR("Null render pass pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    // TODO

    return {.success = true};
}

void hc_destroy_raster_pipeline(HCRasterPipeline* raster_pipeline) {
    if (!raster_pipeline) {
        HC_WARN("Null render pass pointer");
        return;
    }
    const auto device_id = raster_pipeline->device;
    auto device_result = hc::render::device_at(device_id);
    if (!device_result) {
        return;
    }

    // TODO

    *raster_pipeline = {};
}
