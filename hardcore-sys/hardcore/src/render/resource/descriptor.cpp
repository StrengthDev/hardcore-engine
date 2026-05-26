
#include <pch.hpp>

#include "descriptor.hpp"

#include <core/log.hpp>
#include <util/number.hpp>

static Sz size_of(HCPrimitive) {
    // switch (primitive) {
    // case HCPrimitive_U8:
    // case HCPrimitive_I8:
    //     return 1;
    // case HCPrimitive_U16:
    // case HCPrimitive_I16:
    //     return 2;
    // case HCPrimitive_U32:
    // case HCPrimitive_I32:
    // case HCPrimitive_F32:
    // case HCPrimitive_B32:
    //     return 4;
    // case HCPrimitive_U64:
    // case HCPrimitive_I64:
    // case HCPrimitive_F64:
    //     return 8;
    // }

    return 0;
}

static Sz count_of(HCComposition composition) {
    switch (composition) {
    case HCComposition_Scalar:
        return 1;
    case HCComposition_Vec2:
        return 2;
    case HCComposition_Vec3:
        return 3;
    case HCComposition_Vec4:
        return 4;
    case HCComposition_Mat2x2:
        return 4;
    case HCComposition_Mat2x3:
        return 6;
    case HCComposition_Mat2x4:
        return 8;
    case HCComposition_Mat3x2:
        return 6;
    case HCComposition_Mat3x3:
        return 9;
    case HCComposition_Mat3x4:
        return 12;
    case HCComposition_Mat4x2:
        return 8;
    case HCComposition_Mat4x3:
        return 12;
    case HCComposition_Mat4x4:
        return 16;
    }

    HC_ERROR("Invalid composition value");
    return 0;
}

// TODO this may need to be revised based on the alignment https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout
static Sz size_of(const HCField* fields, Sz count) {
    Sz total = 0;
    for (Sz i = 0; i < count; ++i) {
        total += size_of(fields[i].kind) * count_of(fields[i].composition);
    }
    return total;
}

namespace hc::render {
    std::vector<HCTypeDescriptor> const& Descriptor2::data_vec() const noexcept {
        return this->descriptor_data;
    }

    Sz Descriptor2::size() const noexcept {
        return this->total_size;
    }

    Descriptor2::Descriptor2(std::vector<HCTypeDescriptor>&& descriptor_data, Sz total_size) noexcept
        : descriptor_data(descriptor_data), total_size(total_size) {}

    Descriptor2::Descriptor2(std::pair<std::vector<HCTypeDescriptor>, Sz>&& pair) noexcept
        : Descriptor2(std::move(pair.first), pair.second) {}

    Sz size_of(HCPrimitive primitive) {
        return ::size_of(primitive);
    }

    Descriptor::Descriptor(const HCDescriptor& descriptor)
        : fields(descriptor.fields, descriptor.fields + descriptor.field_count) {}

    Sz Descriptor::size() const noexcept {
        return size_of(this->fields.data(), this->fields.size());
    }
}
