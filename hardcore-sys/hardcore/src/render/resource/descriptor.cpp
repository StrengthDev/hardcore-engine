#include <pch.hpp>

#include <core/log.hpp>
#include <util/number.hpp>
#include <render/descriptor.h>

#include "descriptor.hpp"

Sz size_of(HCPrimitive primitive) {
    switch (primitive) {
        case U8:
        case I8:
            return 1;
        case U16:
        case I16:
            return 2;
        case U32:
        case I32:
        case F32:
            return 4;
        case U64:
        case I64:
        case F64:
            return 8;
    }
}

Sz count_of(HCComposition composition) {
    switch (composition) {
        case Scalar:
            return 1;
        case Vec2:
            return 2;
        case Vec3:
            return 3;
        case Vec4:
            return 4;
    }
}

Sz size_of(const HCField *fields, Sz count) {
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

    auto *fields = static_cast<HCField *>(std::malloc(sizeof(HCField) * field_count));
    return {.fields = fields, .field_count = fields ? field_count : 0, .alignment = HCAlignment::Unknown};
}

void hc_destroy_descriptor(HCDescriptor *descriptor) {
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

Sz hc_descriptor_size(const HCDescriptor *descriptor) {
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

    Descriptor::Descriptor(const HCDescriptor &descriptor) :
            fields(descriptor.fields, descriptor.fields + descriptor.field_count), alignment(descriptor.alignment) {
    }

    Sz Descriptor::size() const noexcept {
        return size_of(this->fields.data(), this->fields.size());
    }
}
