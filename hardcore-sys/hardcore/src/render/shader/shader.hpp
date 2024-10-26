#pragma once

#include <render/shader.h>

#include <util/number.hpp>
#include <util/result.hpp>

#include <vector>

namespace hc::render {
    enum class ShaderResult : u8 {
        Success = 0,
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

        std::vector<u32> bytecode;
        HCShaderStage stage;

        friend HCShader (::hc_create_shader)(const u32* bytecode, size_t size, HCShaderStage stage);
    };
}
