#pragma once

#include <core/result.h>

#include <vulkan/vulkan.h>

#include <expected>

namespace hc {
    class Error {
    public:
        Error(HCError error)
            : error(error) {}

        /**
         * Convert a Vulkan error result into the `Error` type.
         *
         * @param result The Vulkan result value to be converted.
         * @return The converted error.
         */
        Error(VkResult result);

        [[nodiscard]] HCError value() const noexcept { return this->error; }

        template<typename T>
        operator std::expected<T, Error>() const noexcept { return std::unexpected(this->error); }

        operator HCResult() const noexcept { return { .error = this->error, .success = false }; }

    private:
        HCError error;
    };
}
