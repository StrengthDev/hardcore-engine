#pragma once

#include "../resource/descriptor.hpp"

#include <render/shader.h>

#include <core/error.hpp>
#include <util/number.hpp>

#include <expected>
#include <vector>

namespace hc::render {
    struct ShaderInput {
        BasicDescriptor descriptor;
        u32 location;
    };

    struct DescriptorLocation {
        u32 set;
        u32 binding;

        bool operator==(DescriptorLocation const& other) const noexcept;
    };
}

template<>
struct std::hash<hc::render::DescriptorLocation> {
    std::size_t operator()(const hc::render::DescriptorLocation& location) const noexcept;
};

namespace hc::render {
    class Shader {
    public:
        Shader(const Shader&) = delete;

        Shader& operator=(const Shader&) = delete;

        Shader(Shader&&) = default;

        Shader& operator=(Shader&&) = default;

        [[nodiscard]]
        static std::expected<Shader, Error> create(std::vector<u32>&& bytecode, HCShaderStage stage);

        VkShaderStageFlagBits stage_flag() const noexcept;
        char const* entrypoint_str() const noexcept;
        std::unordered_map<DescriptorLocation, DescriptorBinding> const& bindings() const noexcept;
        std::vector<ShaderInput> const& inputs_vec() const noexcept;

        std::vector<u32> const& bytecode_vec() const noexcept;

    private:
        Shader() = default;

        std::expected<void, Error> reflect();

        std::vector<u32> bytecode;
        HCShaderStage stage;
        std::string entrypoint;

        std::vector<ShaderInput> inputs;

        std::unordered_map<DescriptorLocation, DescriptorBinding> bindings_map;

        friend HCResult (::hc_create_shader)(HCShader* shader, const u32* bytecode, size_t size, HCShaderStage stage);
    };
}
