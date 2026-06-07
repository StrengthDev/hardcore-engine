
#include <pch.hpp>

#include <render/resource/texture.h>

#include "../../validation.hpp"

#include <render/resource/texture.hpp>

#include <core/log.hpp>
#include <render/renderer.hpp>

#include <util/number.hpp>

bool hc_validate_texture_format(HCTextureFormat const* format) {
    if (!format) {
        return false;
    }

    return hc::render::texture::to_vk_format(*format);
}

HCResult hc_new_texture(
    HCTexture* texture,
    u32 device,
    HCTextureDimensions dims,
    HCTextureFormat format,
    u32 mip_levels,
    HCTextureSampleCount sample_count,
    bool render_target
) {
    HC_VALIDATE_PTR_RE(texture, "texture");

    VkImageType image_type;
    VkExtent3D extent;
    u32 array_layers;
    switch (dims.texture_type) {
    case HCTextureType_Texture1D:
        HC_VALIDATE_RE(dims.dims.dims1D.width > 0, "Texture width must be greater than 0");
        HC_VALIDATE_RE(dims.dims.dims1D.layers > 0, "Texture must have at least 1 layer");

        image_type = VK_IMAGE_TYPE_1D;
        extent = { dims.dims.dims1D.width, 1, 1 };
        array_layers = dims.dims.dims1D.layers;
        break;
    case HCTextureType_Texture2D:
        HC_VALIDATE_RE(dims.dims.dims2D.width > 0, "Texture width must be greater than 0");
        HC_VALIDATE_RE(dims.dims.dims2D.height > 0, "Texture height must be greater than 0");
        HC_VALIDATE_RE(dims.dims.dims2D.layers > 0, "Texture must have at least 1 layer");

        image_type = VK_IMAGE_TYPE_2D;
        extent = { dims.dims.dims2D.width, dims.dims.dims2D.height, 1 };
        array_layers = dims.dims.dims2D.layers;
        break;
    case HCTextureType_Texture3D:
        HC_VALIDATE_RE(dims.dims.dims3D.width > 0, "Texture width must be greater than 0");
        HC_VALIDATE_RE(dims.dims.dims3D.height > 0, "Texture height must be greater than 0");
        HC_VALIDATE_RE(dims.dims.dims3D.depth > 0, "Texture depth must be greater than 0");

        image_type = VK_IMAGE_TYPE_3D;
        extent = { dims.dims.dims3D.width, dims.dims.dims3D.height, dims.dims.dims3D.depth };
        array_layers = 1;
        break;
    default:
        HC_ERROR("Invalid texture type");
        return hc::Error(HCError_InvalidParams);
    }

    // HC_VALIDATE_RE(
    //     image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT && image_info.arrayLayers >= 6,
    //     "Cube compatible textures must have at least 6 layers"
    // );
    // HC_VALIDATE_RE(
    //     std::has_single_bit(std::bit_cast<u32>(image_info.samples)),
    //     "Texture must have exactly 1 sample count bit set"
    // );

    HC_VALIDATE_RE(mip_levels > 0, "Texture must have at least one mip level");

    VkFormat const* format_ptr = hc::render::texture::to_vk_format(format);
    HC_VALIDATE_RE(format_ptr, "Unsupported texture format");

    VkImageCreateFlags flags = 0;

    HC_VALIDATE_RE(
        sample_count == HCTextureSampleCount_SC1
        || (image_type == VK_IMAGE_TYPE_2D && mip_levels == 1 && !(flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)),
        "Multiple samples are only supported for 2D textures with a single mip level, and are incompatible with "
        "cube textures"
    );

    VkSampleCountFlagBits samples;

    switch (sample_count) {
    case HCTextureSampleCount_SC1:
        samples = VK_SAMPLE_COUNT_1_BIT;
        break;
    case HCTextureSampleCount_SC2:
        samples = VK_SAMPLE_COUNT_2_BIT;
        break;
    case HCTextureSampleCount_SC4:
        samples = VK_SAMPLE_COUNT_4_BIT;
        break;
    case HCTextureSampleCount_SC8:
        samples = VK_SAMPLE_COUNT_8_BIT;
        break;
    case HCTextureSampleCount_SC16:
        samples = VK_SAMPLE_COUNT_16_BIT;
        break;
    case HCTextureSampleCount_SC32:
        samples = VK_SAMPLE_COUNT_32_BIT;
        break;
    case HCTextureSampleCount_SC64:
        samples = VK_SAMPLE_COUNT_64_BIT;
        break;
    default:
        HC_ERROR("Invalid sample count");
        return hc::Error(HCError_InvalidParams);
    }

    VkImageUsageFlags usage = 0;
    if (render_target) {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .imageType = image_type,
        .format = *format_ptr,
        .extent = extent,
        .mipLevels = mip_levels,
        .arrayLayers = array_layers,
        .samples = samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto device_result = hc::render::device_at(device);
    if (!device_result) {
        return device_result.error();
    }

    auto texture_result = (*device_result)->create_texture(image_info);
    if (!texture_result) {
        return texture_result.error();
    }
    auto params = *texture_result;

    texture->id = params.id;
    texture->size = params.size;
    texture->device = device;

    return { .success = true };
}

void hc_destroy_texture(HCTexture* texture) {
    HC_VALIDATE_PTR(texture, "texture");
    HC_VALIDATE(texture->size != 0, "Attempted to destroy invalid texture");

    const auto device_id = texture->device;
    auto device_result = hc::render::device_at(device_id);
    if (!device_result) {
        return;
    }

    (*device_result)->destroy_texture(texture->id);
    *texture = {
        .id = std::numeric_limits<u64>::max(),
        .size = 0,
        .device = std::numeric_limits<u32>::max(),
    };
}
