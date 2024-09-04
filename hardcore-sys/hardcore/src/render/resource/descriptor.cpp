#include <pch.hpp>

#include <core/log.hpp>
#include <util/number.hpp>
#include <render/descriptor.h>

#include "descriptor.hpp"

HCDescriptor hc_create_descriptor(u32 field_count) {
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

namespace hc::render::resource {
    Descriptor::Descriptor(const HCDescriptor &descriptor) :
            fields(descriptor.fields, descriptor.fields + descriptor.field_count), alignment(descriptor.alignment) {
    }
}
