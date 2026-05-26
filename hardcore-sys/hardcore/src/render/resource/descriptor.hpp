
#pragma once

#include <render/resource/descriptor.h>

#include <util/number.hpp>

#include <vector>

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
        InputAttachment,
        AccelerationStructure,
    };

    class Descriptor2 {
    public:
        [[nodiscard]] std::vector<HCTypeDescriptor> const& data_vec() const noexcept;
        [[nodiscard]] Sz size() const noexcept;

    private:
        Descriptor2() = default;
        Descriptor2(std::vector<HCTypeDescriptor>&& descriptor_data, Sz total_size) noexcept;
        Descriptor2(std::pair<std::vector<HCTypeDescriptor>, Sz>&& pair) noexcept;

        std::vector<HCTypeDescriptor> descriptor_data;
        Sz total_size = 0;

        friend class Shader;
    };

    // TODO change name
    struct DescriptorBinding {
        std::string name;
        DescriptorType type;
        Descriptor2 descriptor;
    };

    Sz size_of(HCPrimitive primitive);

    class Descriptor {
    public:
        Descriptor() = default;

        explicit Descriptor(const HCDescriptor& descriptor);

        [[nodiscard]] Sz size() const noexcept;

        std::vector<HCField> fields;
    };
}
