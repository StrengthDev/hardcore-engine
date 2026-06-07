
#pragma once

#include "../shader/shader.hpp"

#include <core/error.hpp>
#include <util/number.hpp>

#include <expected>
#include <span>
#include <vector>

namespace hc::render::pipeline {
    class PushConstantsProcessor {
    public:
        PushConstantsProcessor() = default;

        static std::expected<PushConstantsProcessor, Error> create(std::optional<PushConstant> const& push_constants);

        [[nodiscard]] Sz push_constants_size() const noexcept;

        [[nodiscard]]
        std::expected<void, Error> operator()(std::vector<u8>& buffer, std::span<void const*> const& constant_ptrs) const noexcept;

    private:
        enum class PushConstantType : u8 {
            PlainData,
            BufferAddress,
        };

        struct PushConstant {
            u32 size;
            u32 offset;
            PushConstantType type;
        };

        std::vector<PushConstant> constants;

        Sz buffer_size = 0;
    };
}
