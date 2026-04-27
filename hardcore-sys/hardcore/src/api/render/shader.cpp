
#include <pch.hpp>

#include <render/shader.h>

#include <render/shader/shader.hpp>

#include <core/error.hpp>
#include <core/log.hpp>

HCResult hc_create_shader(HCShader* shader, const u32* bytecode, size_t size, HCShaderStage stage) {
    if (!shader) {
        HC_ERROR("Null shader pointer");
        return hc::Error(HCError_InvalidParams);
    }

    if (!bytecode) {
        HC_ERROR("Null bytecode pointer");
        return hc::Error(HCError_InvalidParams);
    }

    if (!size) {
        HC_ERROR("Invalid bytecode length");
        return hc::Error(HCError_InvalidParams);
    }

    auto shader_result = hc::render::Shader::create(std::vector(bytecode, bytecode + size), stage);
    if (!shader_result) {
        return shader_result.error();
    }

    auto* shader_ptr = new hc::render::Shader;
    *shader_ptr = *std::move(shader_result);
    *shader = {.inner = shader_ptr};

    return {.success = true};
}

void hc_destroy_shader(HCShader* shader) {
    if (shader && shader->inner) {
        delete static_cast<hc::render::Shader*>(shader->inner);
        shader->inner = nullptr;
    }
}
