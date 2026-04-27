
#include <pch.hpp>

#include <render/resource/texture.h>

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

HCResult hc_create_texture(
    HCTexture* texture,
    u32 device,
    HCTextureDimensions dims,
    HCTextureFormat format,
    u32 mip_levels,
    HCTextureSampleCount sample_count
) {
    if (!texture) {
        HC_ERROR("Null texture pointer");
        return {.error = HCError_InvalidParams, .success = false};
    }

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags;

    switch (dims.texture_type) {
    case HCTextureType_Texture1D:
        if (dims.dims.dims1D.width < 1) {
            HC_ERROR("Texture width must be greater than 0");
            return {.error = HCError_InvalidParams, .success = false};
        }

        if (dims.dims.dims1D.layers < 1) {
            HC_ERROR("Texture must have at least 1 layer");
            return {.error = HCError_InvalidParams, .success = false};
        }
        image_info.imageType = VK_IMAGE_TYPE_1D;
        image_info.extent = {dims.dims.dims1D.width, 1, 1};
        image_info.arrayLayers = dims.dims.dims1D.layers;
        break;
    case HCTextureType_Texture2D:
        if (dims.dims.dims2D.width < 1) {
            HC_ERROR("Texture width must be greater than 0");
            return {.error = HCError_InvalidParams, .success = false};
        }

        if (dims.dims.dims2D.height < 1) {
            HC_ERROR("Texture height must be greater than 0");
            return {.error = HCError_InvalidParams, .success = false};
        }

        if (dims.dims.dims2D.layers < 1) {
            HC_ERROR("Texture must have at least 1 layer");
            return {.error = HCError_InvalidParams, .success = false};
        }

        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent = {dims.dims.dims2D.width, dims.dims.dims2D.height, 1};
        image_info.arrayLayers = dims.dims.dims2D.layers;
        break;
    case HCTextureType_Texture3D:
        if (dims.dims.dims3D.width < 1) {
            HC_ERROR("Texture width must be greater than 0");
            return {.error = HCError_InvalidParams, .success = false};
        }

        if (dims.dims.dims3D.height < 1) {
            HC_ERROR("Texture height must be greater than 0");
            return {.error = HCError_InvalidParams, .success = false};
        }

        if (dims.dims.dims3D.depth < 1) {
            HC_ERROR("Texture depth must be greater than 0");
            return {.error = HCError_InvalidParams, .success = false};
        }

        image_info.imageType = VK_IMAGE_TYPE_3D;
        image_info.extent = {dims.dims.dims3D.width, dims.dims.dims3D.height, dims.dims.dims3D.depth};
        image_info.arrayLayers = 1;
        break;
    default:
        HC_ERROR("Invalid texture type");
        return {.error = HCError_InvalidParams, .success = false};
    }

    VkFormat const* format_ptr = hc::render::texture::to_vk_format(format);
    if (!format_ptr) {
        HC_ERROR("Unsupported texture format");
        return {.error = HCError_InvalidParams, .success = false};
    }

    image_info.format = *format_ptr;

    if (mip_levels < 1) {
        HC_ERROR("Texture must have at least one mip level");
        return {.error = HCError_InvalidParams, .success = false};
    }

    image_info.mipLevels = mip_levels;

    if (sample_count != HCTextureSampleCount_SC1
        && (image_info.imageType != VK_IMAGE_TYPE_2D || mip_levels != 1 || image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)) {
        HC_ERROR(
            "Multiple samples are only supported for 2D textures with a single mip level, and are incompatible with "
            "cube textures"
        );
        return {.error = HCError_InvalidParams, .success = false};
    }

    switch (sample_count) {
    case HCTextureSampleCount_SC1:
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        break;
    case HCTextureSampleCount_SC2:
        image_info.samples = VK_SAMPLE_COUNT_2_BIT;
        break;
    case HCTextureSampleCount_SC4:
        image_info.samples = VK_SAMPLE_COUNT_4_BIT;
        break;
    case HCTextureSampleCount_SC8:
        image_info.samples = VK_SAMPLE_COUNT_8_BIT;
        break;
    case HCTextureSampleCount_SC16:
        image_info.samples = VK_SAMPLE_COUNT_16_BIT;
        break;
    case HCTextureSampleCount_SC32:
        image_info.samples = VK_SAMPLE_COUNT_32_BIT;
        break;
    case HCTextureSampleCount_SC64:
        image_info.samples = VK_SAMPLE_COUNT_64_BIT;
        break;
    default:
        HC_ERROR("Invalid sample count");
        return {.error = HCError_InvalidParams, .success = false};
    }

    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = nullptr;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

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

    return {.success = true};
}

void hc_destroy_texture(HCTexture* texture) {
    if (!texture) {
        HC_WARN("Null texture pointer");
        return;
    }

    if (!texture->size) {
        HC_WARN("Attempted to destroy invalid texture");
        return;
    }

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
