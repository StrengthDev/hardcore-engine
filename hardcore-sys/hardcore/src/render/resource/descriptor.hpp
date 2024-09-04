#pragma once

#include <render/descriptor.h>

namespace hc::render::resource {
    class Descriptor {
    public:
        explicit Descriptor(const HCDescriptor &descriptor);

    private:
        std::vector<HCField> fields;
        HCAlignment alignment;
    };
}
