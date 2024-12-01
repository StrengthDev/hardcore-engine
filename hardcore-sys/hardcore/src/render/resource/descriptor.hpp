#pragma once

#include <util/number.hpp>
#include <render/descriptor.h>

namespace hc::render::resource {
    Sz size_of(HCPrimitive primitive);

    class Descriptor {
    public:
        Descriptor() = default;

        explicit Descriptor(const HCDescriptor& descriptor);

        [[nodiscard]] Sz size() const noexcept;

        std::vector<HCField> fields;
    };
}
