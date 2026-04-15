#pragma once

#include <util/number.hpp>
#include <../../../include/render/resource/descriptor.h>

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

        bool operator==(const ArrayDescriptor& other) const noexcept = default;
    };

    struct BasicDescriptor {
        HCPrimitive primitive_type;
        u32 primitive_size; // In bits.
        HCComposition composition;
        u32 matrix_stride; // In bytes.

        bool operator==(const BasicDescriptor& other) const noexcept = default;
    };

    struct MemberDescriptor;

    struct StructDescriptor {
        std::vector<std::unique_ptr<MemberDescriptor>> members;

        StructDescriptor() = default;
        StructDescriptor(StructDescriptor&&) = default;
        StructDescriptor(StructDescriptor const& other);

        StructDescriptor& operator=(StructDescriptor&&) = default;

        bool operator==(const StructDescriptor& other) const noexcept;
    };

    typedef std::variant<
        BasicDescriptor, StructDescriptor, ArrayDescriptor<BasicDescriptor>, ArrayDescriptor<StructDescriptor>
    > PODDescriptor;

    struct MemberDescriptor {
        PODDescriptor type_descriptor;
        u32 offset;

        bool operator==(const MemberDescriptor& other) const noexcept = default;
    };

    // TODO change name
    struct DescriptorBinding {
        std::string name;
        DescriptorType type;
        std::variant<std::monostate, PODDescriptor> descriptor;

        bool operator==(const DescriptorBinding& other) const noexcept = default;
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
