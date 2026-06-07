#pragma once

#include "../resource/descriptor.hpp"

#include <render/shader.h>

#include <core/error.hpp>
#include <util/number.hpp>

#include <expected>
#include <numeric>
#include <optional>
#include <vector>

namespace hc::render {
    struct ShaderInput {
        Descriptor2 descriptor;
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
    struct PushConstant {
        Descriptor2 descriptor;
        u32 offset;
    };

    class Shader {
    public:
        Shader(const Shader&) = delete;

        Shader& operator=(const Shader&) = delete;

        Shader(Shader&&) noexcept = default;

        Shader& operator=(Shader&&) = default;

        [[nodiscard]] static std::expected<Shader, Error> create(std::vector<u32>&& bytecode, HCShaderStage stage);

        [[nodiscard]] VkShaderStageFlagBits stage_flag() const noexcept;
        [[nodiscard]] char const* entrypoint_str() const noexcept;
        [[nodiscard]] std::unordered_map<DescriptorLocation, DescriptorBinding> const& bindings() const noexcept;
        [[nodiscard]] std::vector<ShaderInput> const& inputs() const noexcept;
        [[nodiscard]] std::unordered_map<u32, Descriptor2> const& outputs() const noexcept;
        [[nodiscard]] std::optional<PushConstant> const& push_constants() const noexcept;

        [[nodiscard]] std::vector<u32> const& bytecode_vec() const noexcept;

    private:
        Shader() = default;

        std::expected<void, Error> reflect();

        [[nodiscard]]
        static DescriptorBinding create_binding_struct(
            std::string&& name,
            DescriptorType type,
            std::vector<HCTypeDescriptor>&& descriptor_data,
            Sz total_size
        );

        std::vector<u32> bytecode;
        HCShaderStage stage = std::numeric_limits<HCShaderStage>::max();
        std::string entrypoint;

        std::vector<ShaderInput> input_vec;
        std::unordered_map<u32, Descriptor2> output_map;
        std::optional<PushConstant> push_constants_descriptor;

        std::unordered_map<DescriptorLocation, DescriptorBinding> bindings_map;

        friend HCResult (::hc_new_shader)(HCShader* shader, const u32* bytecode, size_t size, HCShaderStage stage);
    };
}
