#pragma once

#include <render/resource/descriptor.hpp>

namespace hc::render {
    enum class DescriptorType : u8 {
        Sampler,
        CombinedImageSampler,
        SampledImage,
        StorageImage,
        UniformTexelBuffer,
        StorageTexelBuffer,
        UniformBuffer,
        StorageBuffer,
        UniformBufferDynamic,
        StorageBufferDynamic,
        InputAttachment,
        AccelerationStructure,
    };

    struct DescriptorBinding {
        std::string name;
        DescriptorType type;
        resource::Descriptor descriptor;
    };
}
