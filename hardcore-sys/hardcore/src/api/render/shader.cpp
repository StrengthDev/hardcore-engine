
#include <pch.hpp>

#include <render/shader.h>

#include "../validation.hpp"

#include <render/shader/shader.hpp>

#include <core/error.hpp>
#include <core/log.hpp>

HCResult hc_new_shader(HCShader* shader, const u32* bytecode, size_t size, HCShaderStage stage) {
    HC_VALIDATE_PTR_RE(shader, "shader");
    HC_VALIDATE_PTR_RE(bytecode, "bytecode");
    HC_VALIDATE_RE(size > 0, "Invalid bytecode length");

    auto shader_result = hc::render::Shader::create(std::vector(bytecode, bytecode + size), stage);
    if (!shader_result) {
        return shader_result.error();
    }

    auto* shader_ptr = new hc::render::Shader;
    *shader_ptr = *std::move(shader_result);
    *shader = { .inner = shader_ptr };

    return { .success = true };
}

void hc_destroy_shader(HCShader* shader) {
    HC_VALIDATE_PTR(shader, "shader");
    HC_VALIDATE_PTR(shader->inner, "shader data");

    delete static_cast<hc::render::Shader*>(shader->inner);
    shader->inner = nullptr;
}

HCResult hc_shader_push_constants(HCShader const* shader, HCTypeDescriptor* descriptor_data, uint32_t* data_count) {
    HC_VALIDATE_PTR_RE(shader, "shader");
    HC_VALIDATE_PTR_RE(data_count, "data count");

    hc::render::Shader const& shader_ref = *static_cast<hc::render::Shader const*>(shader->inner);
    auto push_constants = shader_ref.push_constants();

    if (!descriptor_data) {
        if (push_constants) {
            *data_count = static_cast<u32>(push_constants->descriptor.data_vec().size());
        } else {
            *data_count = 0;
        }
    } else {
        HC_VALIDATE_RE(*data_count == push_constants->descriptor.data_vec().size(), "Incorrect descriptor data count value");

        if (*data_count != 0) {
            std::memcpy(
                descriptor_data,
                push_constants->descriptor.data_vec().data(),
                *data_count * sizeof(HCTypeDescriptor)
            );
        }
    }

    return { .success = true };
}
