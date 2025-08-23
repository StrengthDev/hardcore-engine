#pragma once

#include <util/number.hpp>
#include <render/descriptor.h>

#include <memory>
#include <variant>

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

    template<typename ElementDescriptor>
    struct ArrayDescriptor {
        ElementDescriptor element;
        u32 count;
        u32 stride;
    };

    struct BasicDescriptor {
        HCPrimitive primitive_type;
        u32 primitive_size; // In bits.
        HCComposition composition;
        u32 matrix_stride; // In bytes.
    };

    struct MemberDescriptor;

    struct StructDescriptor {
        std::vector<std::unique_ptr<MemberDescriptor>> members;
    };

    typedef std::variant<
        BasicDescriptor, StructDescriptor, ArrayDescriptor<BasicDescriptor>, ArrayDescriptor<StructDescriptor>
    > PODDescriptor;

    struct MemberDescriptor {
        PODDescriptor type_descriptor;
        u32 offset;
    };

    struct DescriptorBinding {
        std::string name;
        DescriptorType type;
        std::variant<std::monostate, PODDescriptor> descriptor;
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
