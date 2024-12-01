#pragma once

#include "descriptor_binding.hpp"

#include <render/shader.h>

#include <util/number.hpp>
#include <util/result.hpp>

#include <vector>

namespace hc::render {
    enum class ShaderResult : u8 {
        Success = 0,
        FailedReflection,
    };

    class Shader {
    public:
        Shader(const Shader&) = delete;

        Shader& operator=(const Shader&) = delete;

        Shader(Shader&&) = default;

        Shader& operator=(Shader&&) = default;

        static Result<Shader, ShaderResult> create(std::vector<u32>&& bytecode, HCShaderStage stage);

    private:
        Shader() = default;

        static ShaderResult reflect(Shader& shader);

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

        friend HCShader (::hc_create_shader)(const u32* bytecode, size_t size, HCShaderStage stage);
    };
}
