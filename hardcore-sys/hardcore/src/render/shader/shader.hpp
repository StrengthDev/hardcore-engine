#pragma once

#include "../resource/descriptor.hpp"

#include <render/shader.h>

#include <core/error.hpp>
#include <util/number.hpp>

#include <expected>
#include <vector>

namespace hc::render {
    class Shader {
    public:
        Shader(const Shader&) = delete;

        Shader& operator=(const Shader&) = delete;

        Shader(Shader&&) = default;

        Shader& operator=(Shader&&) = default;

        [[nodiscard]]
        static std::expected<Shader, Error> create(std::vector<u32>&& bytecode, HCShaderStage stage);

    private:
        Shader() = default;

        std::expected<void, Error> reflect();

        std::vector<u32> bytecode;
        HCShaderStage stage;
        std::string entrypoint;

        /**
        * @brief Custom hash function for the binding location type.
        */
        struct LocationHash {
            std::size_t operator()(const std::pair<u32, u32>& output) const noexcept;
        };

        std::unordered_map<std::pair<u32, u32>, DescriptorBinding, LocationHash> bindings;

        friend HCResult (::hc_create_shader)(HCShader* shader, const u32* bytecode, size_t size, HCShaderStage stage);
    };
}
