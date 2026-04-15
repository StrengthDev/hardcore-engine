
#pragma once

#include "descriptor_pool.hpp"

#include "../shader/shader.hpp"

#include <vulkan/vulkan.h>

#include <core/error.hpp>
#include <util/uncopyable.hpp>

namespace hc::render {
    // TODO move this
    // TODO this name is bad, change to interpretation or something
    enum class VertexNumericFormat : u8 {
        UNorm,
        SNorm,
        UScaled,
        SScaled,
        UInt,
        SInt,
        UFloat,
        SFloat,
    };

    struct InputBufferAttribute {
        u32 location;
        u8 size; //!< Component size, in bytes.
        VertexNumericFormat format;
        HCComposition composition;
        u32 offset; //!< In bytes, relative to binding stride.
    };

    struct InputBufferDescription {
        u32 binding;
        VkVertexInputRate input_rate; //!< Indicates when to advance to the next buffer element, after each vertex or each instance.
        std::vector<InputBufferAttribute> attributes;
        u32 stride; //!< In bytes.
    };

    class RasterPipelineState {
        // this guy will merely receive optionals and help fill the create_info accordingly
        // within the render graph, the pipeline node will keep track of how the state is
        // managed (constant/depends on something like render target/can be arbitrarily modified by user/etc)
    };

    class RasterPipeline {
    public:
        [[nodiscard]]
        static std::expected<RasterPipeline, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkPipelineCache cache,
            std::vector<Shader> const& shaders,
            std::vector<InputBufferDescription> const& buffer_descriptions
        );

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

    private:
        RasterPipeline() = default;

        ExternalHandle<VkPipeline, VK_NULL_HANDLE> handle;
        ExternalHandle<VkPipelineLayout, VK_NULL_HANDLE> layout;

        DescriptorPool descriptor_pool;
    };
}
