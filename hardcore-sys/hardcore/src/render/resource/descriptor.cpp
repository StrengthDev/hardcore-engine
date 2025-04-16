#include <pch.hpp>

#include "descriptor.hpp"

#include <core/log.hpp>
#include <render/descriptor.h>
#include <util/number.hpp>

static Sz size_of(HCPrimitive primitive) {
    switch (primitive) {
    case HCPrimitive_U8:
    case HCPrimitive_I8:
        return 1;
    case HCPrimitive_U16:
    case HCPrimitive_I16:
        return 2;
    case HCPrimitive_U32:
    case HCPrimitive_I32:
    case HCPrimitive_F32:
    case HCPrimitive_B32:
        return 4;
    case HCPrimitive_U64:
    case HCPrimitive_I64:
    case HCPrimitive_F64:
        return 8;
    }

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

HCDescriptor hc_create_descriptor(Sz field_count) {
    if (field_count == 0) {
        HC_ERROR("Invalid field count");
        return {};
    }

    auto* fields = static_cast<HCField*>(std::malloc(sizeof(HCField) * field_count));
    return {.fields = fields, .field_count = fields ? field_count : 0, .alignment = HCAlignment::HCAlignment_Unknown};
}

void hc_destroy_descriptor(HCDescriptor* descriptor) {
    if (!descriptor)
        return;

    if (!descriptor->fields || !descriptor->field_count) {
        HC_WARN("Attempted to destroy invalid descriptor");
        return;
    }

    std::free(descriptor->fields);
    descriptor->fields = nullptr;
    descriptor->field_count = 0;
}

Sz hc_descriptor_size(const HCDescriptor* descriptor) {
    if (!descriptor)
        return 0;

    if (!descriptor->fields || !descriptor->field_count) {
        HC_WARN("Invalid descriptor");
        return 0;
    }

    return size_of(descriptor->fields, descriptor->field_count);
}

namespace hc::render::resource {
    Sz size_of(HCPrimitive primitive) {
        return ::size_of(primitive);
    }

    Descriptor::Descriptor(const HCDescriptor& descriptor)
        : fields(descriptor.fields, descriptor.fields + descriptor.field_count) {
    }

    Sz Descriptor::size() const noexcept {
        return size_of(this->fields.data(), this->fields.size());
    }
}
