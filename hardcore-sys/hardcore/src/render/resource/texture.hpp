#pragma once

#include "../vulkan.hpp"
#include "../device/memory/reference.hpp"

#include <render/resource/texture.h>

#include <core/error.hpp>
#include <util/number.hpp>

#include <expected>
#include <unordered_map>

bool operator==(HCTextureViewParams const& lhs, HCTextureViewParams const& rhs);

template<>
struct std::hash<HCTextureViewParams> {
    std::size_t operator()(HCTextureViewParams const& params) const noexcept;
};

namespace hc::render::texture {
    struct TextureView {
        vk::ImageView handle;
        u32 ref_count;
    };

    class Texture {
    public:
        Texture(device::memory::Ref const& ref, vk::Image&& image, VkImageCreateInfo const& image_info);

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

        [[nodiscard]] device::memory::Ref const& memory_ref() const noexcept;

        [[nodiscard]] VkFormat format() const noexcept;

        [[nodiscard]] std::expected<VkImageView, Error> get_view(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            HCTextureViewParams const& params
        ) noexcept;

        void free_view(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            HCTextureViewParams const& params
        ) noexcept;

    private:
        device::memory::Ref ref;
        vk::Image handle;
        VkImageCreateInfo image_info;

        std::unordered_map<HCTextureViewParams, TextureView> views;
    };

    struct TextureData {
        u64 id;
        Sz size;
    };

    [[nodiscard]] std::expected<vk::Image, Error> create_image(
        VkPhysicalDevice physical_device,
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkImageCreateInfo const& image_info
    );

    [[nodiscard]] std::expected<vk::ImageView, Error> create_image_view(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkImage image,
        VkImageCreateInfo const& image_info,
        HCTextureViewParams const& view_params
    );
}
