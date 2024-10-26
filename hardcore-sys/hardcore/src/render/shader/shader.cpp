#include <pch.hpp>

#include "shader.hpp"

#include <render/shader.h>

namespace hc::render {
    Result<Shader, ShaderResult> Shader::create(std::vector<u32>&& bytecode, HCShaderStage stage) {
        Shader shader;
        shader.bytecode = std::move(bytecode);
        shader.stage = stage;
        return Ok(std::move(shader));
    }
}

HCShader hc_create_shader(const u32* bytecode, size_t size, HCShaderStage stage) {
    if (!bytecode || !size)
        return {.inner = nullptr};

    auto shader_res = hc::render::Shader::create(std::vector<u32>(bytecode, bytecode + size), stage);
    if (!shader_res)
        return {.inner = nullptr};

    auto* shader_ptr = new hc::render::Shader;
    *shader_ptr = std::move(shader_res).ok();

    return {.inner = shader_ptr};
}

void hc_destroy_shader(HCShader* shader) {
    if (shader && shader->inner) {
        delete static_cast<hc::render::Shader*>(shader->inner);
        shader->inner = nullptr;
    }
}

