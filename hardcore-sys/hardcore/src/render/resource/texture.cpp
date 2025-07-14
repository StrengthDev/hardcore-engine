#include <pch.hpp>

#include "texture.hpp"

#include "render/renderer.hpp"
#include "render/util.hpp"

#include <render/texture.h>

#include <util/flow.hpp>
#include <util/number.hpp>

namespace hc::render {
    static HCTexture constexpr INVALID_TEXTURE = {
        .id = std::numeric_limits<u64>::max(),
        .size = 0,
        .device = std::numeric_limits<u32>::max(),
    };

    struct StandardTextureFormat {
        HCTextureComponentFormat component_format : 8;
        HCTextureNumericFormat numeric_format : 8;

        bool operator==(const StandardTextureFormat& other) const {
            return this->component_format == other.component_format && this->numeric_format == other.numeric_format;
        }
    };
};

template<>
struct std::hash<hc::render::StandardTextureFormat> {
    std::size_t operator()(const hc::render::StandardTextureFormat& format) const noexcept {
        static_assert(sizeof(hc::render::StandardTextureFormat) <= sizeof(u32));
        u32 const value = static_cast<u32>(format.component_format) + (static_cast<u32>(format.numeric_format) << 8);
        return std::hash<u32>{}(value);
    }
};

namespace hc::render {
    static std::unordered_map<StandardTextureFormat, VkFormat> const STANDARD_FORMAT_MAP = {
        {{HCTextureComponentFormat_R8, HCTextureNumericFormat_UNorm}, VK_FORMAT_R8_UNORM},
        {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_UNorm}, VK_FORMAT_R8G8_UNORM},
        {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_UNorm}, VK_FORMAT_R8G8B8_UNORM},
        {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_UNorm}, VK_FORMAT_R8G8B8A8_UNORM},
        {{HCTextureComponentFormat_R16, HCTextureNumericFormat_UNorm}, VK_FORMAT_R16_UNORM},
        {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_UNorm}, VK_FORMAT_R16G16_UNORM},
        {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_UNorm}, VK_FORMAT_R16G16B16_UNORM},
        {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_UNorm}, VK_FORMAT_R16G16B16A16_UNORM},
        // {{HCTextureComponentFormat_R32, HCTextureNumericFormat_UNorm}, VK_FORMAT_R32_UNORM},
        // {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_UNorm}, VK_FORMAT_R32G32_UNORM},
        // {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_UNorm}, VK_FORMAT_R32G32B32_UNORM},
        // {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_UNorm}, VK_FORMAT_R32G32B32A32_UNORM},
        // {{HCTextureComponentFormat_R64, HCTextureNumericFormat_UNorm}, VK_FORMAT_R64_UNORM},
        // {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_UNorm}, VK_FORMAT_R64G64_UNORM},
        // {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_UNorm}, VK_FORMAT_R64G64B64_UNORM},
        // {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_UNorm}, VK_FORMAT_R64G64B64A64_UNORM},
        {{HCTextureComponentFormat_R8, HCTextureNumericFormat_SNorm}, VK_FORMAT_R8_SNORM},
        {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_SNorm}, VK_FORMAT_R8G8_SNORM},
        {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_SNorm}, VK_FORMAT_R8G8B8_SNORM},
        {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_SNorm}, VK_FORMAT_R8G8B8A8_SNORM},
        {{HCTextureComponentFormat_R16, HCTextureNumericFormat_SNorm}, VK_FORMAT_R16_SNORM},
        {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_SNorm}, VK_FORMAT_R16G16_SNORM},
        {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_SNorm}, VK_FORMAT_R16G16B16_SNORM},
        {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_SNorm}, VK_FORMAT_R16G16B16A16_SNORM},
        // {{HCTextureComponentFormat_R32, HCTextureNumericFormat_SNorm}, VK_FORMAT_R32_SNORM},
        // {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_SNorm}, VK_FORMAT_R32G32_SNORM},
        // {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_SNorm}, VK_FORMAT_R32G32B32_SNORM},
        // {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_SNorm}, VK_FORMAT_R32G32B32A32_SNORM},
        // {{HCTextureComponentFormat_R64, HCTextureNumericFormat_SNorm}, VK_FORMAT_R64_SNORM},
        // {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_SNorm}, VK_FORMAT_R64G64_SNORM},
        // {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_SNorm}, VK_FORMAT_R64G64B64_SNORM},
        // {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_SNorm}, VK_FORMAT_R64G64B64A64_SNORM},
        {{HCTextureComponentFormat_R8, HCTextureNumericFormat_UScaled}, VK_FORMAT_R8_USCALED},
        {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_UScaled}, VK_FORMAT_R8G8_USCALED},
        {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_UScaled}, VK_FORMAT_R8G8B8_USCALED},
        {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_UScaled}, VK_FORMAT_R8G8B8A8_USCALED},
        {{HCTextureComponentFormat_R16, HCTextureNumericFormat_UScaled}, VK_FORMAT_R16_USCALED},
        {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_UScaled}, VK_FORMAT_R16G16_USCALED},
        {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_UScaled}, VK_FORMAT_R16G16B16_USCALED},
        {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_UScaled}, VK_FORMAT_R16G16B16A16_USCALED},
        // {{HCTextureComponentFormat_R32, HCTextureNumericFormat_UScaled}, VK_FORMAT_R32_USCALED},
        // {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_UScaled}, VK_FORMAT_R32G32_USCALED},
        // {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_UScaled}, VK_FORMAT_R32G32B32_USCALED},
        // {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_UScaled}, VK_FORMAT_R32G32B32A32_USCALED},
        // {{HCTextureComponentFormat_R64, HCTextureNumericFormat_UScaled}, VK_FORMAT_R64_USCALED},
        // {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_UScaled}, VK_FORMAT_R64G64_USCALED},
        // {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_UScaled}, VK_FORMAT_R64G64B64_USCALED},
        // {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_UScaled}, VK_FORMAT_R64G64B64A64_USCALED},
        {{HCTextureComponentFormat_R8, HCTextureNumericFormat_SScaled}, VK_FORMAT_R8_SSCALED},
        {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_SScaled}, VK_FORMAT_R8G8_SSCALED},
        {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_SScaled}, VK_FORMAT_R8G8B8_SSCALED},
        {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_SScaled}, VK_FORMAT_R8G8B8A8_SSCALED},
        {{HCTextureComponentFormat_R16, HCTextureNumericFormat_SScaled}, VK_FORMAT_R16_SSCALED},
        {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_SScaled}, VK_FORMAT_R16G16_SSCALED},
        {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_SScaled}, VK_FORMAT_R16G16B16_SSCALED},
        {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_SScaled}, VK_FORMAT_R16G16B16A16_SSCALED},
        // {{HCTextureComponentFormat_R32, HCTextureNumericFormat_SScaled}, VK_FORMAT_R32_SSCALED},
        // {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_SScaled}, VK_FORMAT_R32G32_SSCALED},
        // {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_SScaled}, VK_FORMAT_R32G32B32_SSCALED},
        // {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_SScaled}, VK_FORMAT_R32G32B32A32_SSCALED},
        // {{HCTextureComponentFormat_R64, HCTextureNumericFormat_SScaled}, VK_FORMAT_R64_SSCALED},
        // {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_SScaled}, VK_FORMAT_R64G64_SSCALED},
        // {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_SScaled}, VK_FORMAT_R64G64B64_SSCALED},
        // {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_SScaled}, VK_FORMAT_R64G64B64A64_SSCALED},
        {{HCTextureComponentFormat_R8, HCTextureNumericFormat_UInt}, VK_FORMAT_R8_UINT},
        {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_UInt}, VK_FORMAT_R8G8_UINT},
        {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_UInt}, VK_FORMAT_R8G8B8_UINT},
        {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_UInt}, VK_FORMAT_R8G8B8A8_UINT},
        {{HCTextureComponentFormat_R16, HCTextureNumericFormat_UInt}, VK_FORMAT_R16_UINT},
        {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_UInt}, VK_FORMAT_R16G16_UINT},
        {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_UInt}, VK_FORMAT_R16G16B16_UINT},
        {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_UInt}, VK_FORMAT_R16G16B16A16_UINT},
        {{HCTextureComponentFormat_R32, HCTextureNumericFormat_UInt}, VK_FORMAT_R32_UINT},
        {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_UInt}, VK_FORMAT_R32G32_UINT},
        {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_UInt}, VK_FORMAT_R32G32B32_UINT},
        {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_UInt}, VK_FORMAT_R32G32B32A32_UINT},
        {{HCTextureComponentFormat_R64, HCTextureNumericFormat_UInt}, VK_FORMAT_R64_UINT},
        {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_UInt}, VK_FORMAT_R64G64_UINT},
        {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_UInt}, VK_FORMAT_R64G64B64_UINT},
        {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_UInt}, VK_FORMAT_R64G64B64A64_UINT},
        {{HCTextureComponentFormat_R8, HCTextureNumericFormat_SInt}, VK_FORMAT_R8_SINT},
        {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_SInt}, VK_FORMAT_R8G8_SINT},
        {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_SInt}, VK_FORMAT_R8G8B8_SINT},
        {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_SInt}, VK_FORMAT_R8G8B8A8_SINT},
        {{HCTextureComponentFormat_R16, HCTextureNumericFormat_SInt}, VK_FORMAT_R16_SINT},
        {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_SInt}, VK_FORMAT_R16G16_SINT},
        {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_SInt}, VK_FORMAT_R16G16B16_SINT},
        {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_SInt}, VK_FORMAT_R16G16B16A16_SINT},
        {{HCTextureComponentFormat_R32, HCTextureNumericFormat_SInt}, VK_FORMAT_R32_SINT},
        {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_SInt}, VK_FORMAT_R32G32_SINT},
        {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_SInt}, VK_FORMAT_R32G32B32_SINT},
        {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_SInt}, VK_FORMAT_R32G32B32A32_SINT},
        {{HCTextureComponentFormat_R64, HCTextureNumericFormat_SInt}, VK_FORMAT_R64_SINT},
        {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_SInt}, VK_FORMAT_R64G64_SINT},
        {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_SInt}, VK_FORMAT_R64G64B64_SINT},
        {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_SInt}, VK_FORMAT_R64G64B64A64_SINT},
        // {{HCTextureComponentFormat_R8, HCTextureNumericFormat_UFloat}, VK_FORMAT_R8_UFLOAT},
        // {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_UFloat}, VK_FORMAT_R8G8_UFLOAT},
        // {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_UFloat}, VK_FORMAT_R8G8B8_UFLOAT},
        // {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_UFloat}, VK_FORMAT_R8G8B8A8_UFLOAT},
        // {{HCTextureComponentFormat_R16, HCTextureNumericFormat_UFloat}, VK_FORMAT_R16_UFLOAT},
        // {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_UFloat}, VK_FORMAT_R16G16_UFLOAT},
        // {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_UFloat}, VK_FORMAT_R16G16B16_UFLOAT},
        // {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_UFloat}, VK_FORMAT_R16G16B16A16_UFLOAT},
        // {{HCTextureComponentFormat_R32, HCTextureNumericFormat_UFloat}, VK_FORMAT_R32_UFLOAT},
        // {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_UFloat}, VK_FORMAT_R32G32_UFLOAT},
        // {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_UFloat}, VK_FORMAT_R32G32B32_UFLOAT},
        // {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_UFloat}, VK_FORMAT_R32G32B32A32_UFLOAT},
        // {{HCTextureComponentFormat_R64, HCTextureNumericFormat_UFloat}, VK_FORMAT_R64_UFLOAT},
        // {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_UFloat}, VK_FORMAT_R64G64_UFLOAT},
        // {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_UFloat}, VK_FORMAT_R64G64B64_UFLOAT},
        // {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_UFloat}, VK_FORMAT_R64G64B64A64_UFLOAT},
        // {{HCTextureComponentFormat_R8, HCTextureNumericFormat_SFloat}, VK_FORMAT_R8_SFloat},
        // {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_SFloat}, VK_FORMAT_R8G8_SFloat},
        // {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_SFloat}, VK_FORMAT_R8G8B8_SFloat},
        // {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_SFloat}, VK_FORMAT_R8G8B8A8_SFloat},
        // {{HCTextureComponentFormat_R16, HCTextureNumericFormat_SFloat}, VK_FORMAT_R16_SFloat},
        // {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_SFloat}, VK_FORMAT_R16G16_SFloat},
        // {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_SFloat}, VK_FORMAT_R16G16B16_SFloat},
        // {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_SFloat}, VK_FORMAT_R16G16B16A16_SFloat},
        // {{HCTextureComponentFormat_R32, HCTextureNumericFormat_SFloat}, VK_FORMAT_R32_SFloat},
        // {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_SFloat}, VK_FORMAT_R32G32_SFloat},
        // {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_SFloat}, VK_FORMAT_R32G32B32_SFloat},
        // {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_SFloat}, VK_FORMAT_R32G32B32A32_SFloat},
        // {{HCTextureComponentFormat_R64, HCTextureNumericFormat_SFloat}, VK_FORMAT_R64_SFloat},
        // {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_SFloat}, VK_FORMAT_R64G64_SFloat},
        // {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_SFloat}, VK_FORMAT_R64G64B64_SFloat},
        // {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_SFloat}, VK_FORMAT_R64G64B64A64_SFloat},
        {{HCTextureComponentFormat_R8, HCTextureNumericFormat_SRGB}, VK_FORMAT_R8_SRGB},
        {{HCTextureComponentFormat_R8G8, HCTextureNumericFormat_SRGB}, VK_FORMAT_R8G8_SRGB},
        {{HCTextureComponentFormat_R8G8B8, HCTextureNumericFormat_SRGB}, VK_FORMAT_R8G8B8_SRGB},
        {{HCTextureComponentFormat_R8G8B8A8, HCTextureNumericFormat_SRGB}, VK_FORMAT_R8G8B8A8_SRGB},
        // {{HCTextureComponentFormat_R16, HCTextureNumericFormat_SRGB}, VK_FORMAT_R16_SRGB},
        // {{HCTextureComponentFormat_R16G16, HCTextureNumericFormat_SRGB}, VK_FORMAT_R16G16_SRGB},
        // {{HCTextureComponentFormat_R16G16B16, HCTextureNumericFormat_SRGB}, VK_FORMAT_R16G16B16_SRGB},
        // {{HCTextureComponentFormat_R16G16B16A16, HCTextureNumericFormat_SRGB}, VK_FORMAT_R16G16B16A16_SRGB},
        // {{HCTextureComponentFormat_R32, HCTextureNumericFormat_SRGB}, VK_FORMAT_R32_SRGB},
        // {{HCTextureComponentFormat_R32G32, HCTextureNumericFormat_SRGB}, VK_FORMAT_R32G32_SRGB},
        // {{HCTextureComponentFormat_R32G32B32, HCTextureNumericFormat_SRGB}, VK_FORMAT_R32G32B32_SRGB},
        // {{HCTextureComponentFormat_R32G32B32A32, HCTextureNumericFormat_SRGB}, VK_FORMAT_R32G32B32A32_SRGB},
        // {{HCTextureComponentFormat_R64, HCTextureNumericFormat_SRGB}, VK_FORMAT_R64_SRGB},
        // {{HCTextureComponentFormat_R64G64, HCTextureNumericFormat_SRGB}, VK_FORMAT_R64G64_SRGB},
        // {{HCTextureComponentFormat_R64G64B64, HCTextureNumericFormat_SRGB}, VK_FORMAT_R64G64B64_SRGB},
        // {{HCTextureComponentFormat_R64G64B64A64, HCTextureNumericFormat_SRGB}, VK_FORMAT_R64G64B64A64_SRGB},
        {{HCTextureComponentFormat_B8G8R8, HCTextureNumericFormat_UNorm}, VK_FORMAT_B8G8R8_UNORM},
        {{HCTextureComponentFormat_B8G8R8A8, HCTextureNumericFormat_UNorm}, VK_FORMAT_B8G8R8A8_UNORM},
    };

    struct CompressedTextureFormat {
        HCTextureCompression compression : 8;
        HCTextureNumericFormat numeric_format : 8;
        HCTextureBlockSize block_size : 8;

        bool operator==(const CompressedTextureFormat& other) const {
            return this->compression == other.compression
                && this->numeric_format == other.numeric_format
                && this->block_size == other.block_size;
        }
    };
};

template<>
struct std::hash<hc::render::CompressedTextureFormat> {
    std::size_t operator()(const hc::render::CompressedTextureFormat& format) const noexcept {
        static_assert(sizeof(hc::render::CompressedTextureFormat) <= sizeof(u32));
        u32 const value = static_cast<u32>(format.compression)
            + (static_cast<u32>(format.numeric_format) << 8)
            + (static_cast<u32>(format.block_size) << 16);
        return std::hash<u32>{}(value);
    }
};

namespace hc::render {
    static std::unordered_map<CompressedTextureFormat, VkFormat> const COMPRESSED_FORMAT_MAP = {
        {{HCTextureCompression_BC1, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC1_RGB_UNORM_BLOCK},
        {{HCTextureCompression_BC1, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC1_RGB_SRGB_BLOCK},
        {{HCTextureCompression_BC1_A, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
        {{HCTextureCompression_BC1_A, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
        {{HCTextureCompression_BC2, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC2_UNORM_BLOCK},
        {{HCTextureCompression_BC2, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC2_SRGB_BLOCK},
        {{HCTextureCompression_BC3, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC3_UNORM_BLOCK},
        {{HCTextureCompression_BC3, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC3_SRGB_BLOCK},
        {{HCTextureCompression_BC4, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC4_UNORM_BLOCK},
        {{HCTextureCompression_BC4, HCTextureNumericFormat_SNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC4_SNORM_BLOCK},
        {{HCTextureCompression_BC5, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC5_UNORM_BLOCK},
        {{HCTextureCompression_BC5, HCTextureNumericFormat_SNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC5_SNORM_BLOCK},
        {{HCTextureCompression_BC6, HCTextureNumericFormat_UFloat, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC6H_UFLOAT_BLOCK},
        {{HCTextureCompression_BC6, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC6H_SFLOAT_BLOCK},
        {{HCTextureCompression_BC7, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC7_UNORM_BLOCK},
        {{HCTextureCompression_BC7, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_BC7_SRGB_BLOCK},
        {{HCTextureCompression_ETC2, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK},
        {{HCTextureCompression_ETC2, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
        {{HCTextureCompression_ETC2_A1, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK},
        {{HCTextureCompression_ETC2_A1, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
        {{HCTextureCompression_ETC2_A8, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK},
        {{HCTextureCompression_ETC2_A8, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
        {{HCTextureCompression_EAC_R, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_EAC_R11_UNORM_BLOCK},
        {{HCTextureCompression_EAC_R, HCTextureNumericFormat_SNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_EAC_R11_SNORM_BLOCK},
        {{HCTextureCompression_EAC_RG, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_EAC_R11G11_UNORM_BLOCK},
        {{HCTextureCompression_EAC_RG, HCTextureNumericFormat_SNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_EAC_R11G11_SNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS4x4}, VK_FORMAT_ASTC_4x4_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS5x4}, VK_FORMAT_ASTC_5x4_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS5x5}, VK_FORMAT_ASTC_5x5_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS6x5}, VK_FORMAT_ASTC_6x5_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS6x6}, VK_FORMAT_ASTC_6x6_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS8x5}, VK_FORMAT_ASTC_8x5_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS8x6}, VK_FORMAT_ASTC_8x6_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS8x8}, VK_FORMAT_ASTC_8x8_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS10x5}, VK_FORMAT_ASTC_10x5_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS10x6}, VK_FORMAT_ASTC_10x6_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS10x8}, VK_FORMAT_ASTC_10x8_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS10x10}, VK_FORMAT_ASTC_10x10_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS12x10}, VK_FORMAT_ASTC_12x10_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_UNorm, HCTextureBlockSize_BS12x12}, VK_FORMAT_ASTC_12x12_UNORM_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS4x4}, VK_FORMAT_ASTC_4x4_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS5x4}, VK_FORMAT_ASTC_5x4_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS5x5}, VK_FORMAT_ASTC_5x5_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS6x5}, VK_FORMAT_ASTC_6x5_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS6x6}, VK_FORMAT_ASTC_6x6_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS8x5}, VK_FORMAT_ASTC_8x5_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS8x6}, VK_FORMAT_ASTC_8x6_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS8x8}, VK_FORMAT_ASTC_8x8_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS10x5}, VK_FORMAT_ASTC_10x5_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS10x6}, VK_FORMAT_ASTC_10x6_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS10x8}, VK_FORMAT_ASTC_10x8_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS10x10}, VK_FORMAT_ASTC_10x10_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS12x10}, VK_FORMAT_ASTC_12x10_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SRGB, HCTextureBlockSize_BS12x12}, VK_FORMAT_ASTC_12x12_SRGB_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS4x4}, VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS5x4}, VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS5x5}, VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS6x5}, VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS6x6}, VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS8x5}, VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS8x6}, VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS8x8}, VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS10x5}, VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS10x6}, VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS10x8}, VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS10x10}, VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS12x10}, VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK},
        {{HCTextureCompression_ASTC, HCTextureNumericFormat_SFloat, HCTextureBlockSize_BS12x12}, VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK},
    };

    namespace texture {
        std::expected<VkImage, Error> create_image(
            VkPhysicalDevice physical_device,
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkImageCreateInfo const& image_info
        ) {
            HC_ASSERT(0 < image_info.extent.width, "Texture width must be greater than 0");
            HC_ASSERT(0 < image_info.extent.height, "Texture height must be greater than 0");
            HC_ASSERT(0 < image_info.extent.depth, "Texture depth must be greater than 0");
            HC_ASSERT(0 < image_info.arrayLayers, "Texture must have at least one layer");
            HC_ASSERT(0 < image_info.mipLevels, "Texture must have at least one mip level");
            HC_ASSERT(
                !(image_info.imageType == VK_IMAGE_TYPE_3D && image_info.arrayLayers != 1),
                "3D textures must have exactly 1 layer"
            );
            HC_ASSERT(
                !(image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT && image_info.arrayLayers < 6),
                "Cube compatible textures must have at least 6 layers"
            );
            HC_ASSERT(
                std::has_single_bit(std::bit_cast<u32>(image_info.samples)),
                "Texture must have exactly 1 sample count bit set"
            );

            VkImageFormatProperties format_properties = {};
            VkResult result = vkGetPhysicalDeviceImageFormatProperties(
                physical_device,
                image_info.format,
                image_info.imageType,
                image_info.tiling,
                image_info.usage,
                image_info.flags,
                &format_properties
            );
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to retrieve device limits for image format: " << to_str(result));
                return Error(result);
            }

            if (format_properties.maxExtent.width < image_info.extent.width
                || format_properties.maxExtent.height < image_info.extent.height
                || format_properties.maxExtent.depth < image_info.extent.depth) {
                HC_ERROR(
                    "Texture dimensions are too large, max extent for " << to_str(image_info.format) << " is "
                    << to_str(format_properties.maxExtent)
                );
                return Error(HCError_TextureParamsNotSupported);
            }

            if (format_properties.maxMipLevels < image_info.mipLevels) {
                HC_ERROR(
                    "Texture has too many mip levels, max mip level count for " << to_str(image_info.format) << " is "
                    << format_properties.maxMipLevels
                );
                return Error(HCError_TextureParamsNotSupported);
            }

            if (format_properties.maxArrayLayers < image_info.arrayLayers) {
                HC_ERROR(
                    "Texture array has too many layers, max layer count for " << to_str(image_info.format) << " is "
                    << format_properties.maxArrayLayers
                );
                return Error(HCError_TextureParamsNotSupported);
            }

            if (!(format_properties.sampleCounts & image_info.samples)) {
                std::stringstream available_sample_counts;
                available_sample_counts << "1";
                for (u32 i = 2; i < sizeof(format_properties.sampleCounts) * 8; i++) {
                    auto const count = 1U << i;
                    if (count & format_properties.sampleCounts) {
                        available_sample_counts << ", " << count;
                    }
                }
                HC_ERROR(
                    "Texture has an unsupported sample count for " << to_str(image_info.format)
                    << ", supported sample counts are: " << available_sample_counts.str()
                );
                return Error(HCError_TextureParamsNotSupported);
            }

            VkImage image;
            result = fn_table.vkCreateImage(device, &image_info, nullptr, &image);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create image: " << to_str(result));
                return Error(result);
            }

            return image;
        }

        std::expected<VkImage, Error> create_image_view(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkImage image,
            VkImageCreateInfo image_info
        ) {
            // TODO
            VkImageView image_view;

            VkImageViewCreateInfo view_info = {};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.flags = 0;
            view_info.image = image;
            view_info.format = image_info.format;

            // Setting all components to identity so no swizzling occurs
            view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = image_info.mipLevels;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = image_info.arrayLayers;

            const bool is_cube = image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            HC_ASSERT(
                !is_cube || (is_cube && image_info.imageType == VK_IMAGE_TYPE_2D),
                "Image type must be 2D if image is a cube"
            );
            HC_ASSERT(
                !is_cube || (is_cube && image_info.arrayLayers % 6 == 0),
                "Number of image layers must be a multiple of 6 if image is a cube"
            );

            const bool is_array = is_cube ? 1 < image_info.arrayLayers / 6 : 1 < image_info.arrayLayers;
            HC_ASSERT(!is_array || (is_array && image_info.imageType != VK_IMAGE_TYPE_3D), "3D image arrays are invalid");

            switch (image_info.imageType) {
            case VK_IMAGE_TYPE_1D:
                view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
                break;
            case VK_IMAGE_TYPE_2D:
                if (is_cube) {
                    view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
                } else {
                    view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                }
                break;
            case VK_IMAGE_TYPE_3D:
                view_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
            default: HC_UNREACHABLE("Invalid image type");
            }

            VkResult result = fn_table.vkCreateImageView(device, &view_info, nullptr, &image_view);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create image view");
                return Error(result);
            }

            return Error(HCError_OutOfDeviceMemory);
        }
    }
}

HCTextureFormatID hc_texture_format_id_standard(
    HCTextureComponentFormat component_format,
    HCTextureNumericFormat numeric_format
) {
    auto const format = hc::render::STANDARD_FORMAT_MAP.find({component_format, numeric_format});
    if (format == hc::render::STANDARD_FORMAT_MAP.end()) {
        return std::numeric_limits<HCTextureFormatID>::max();
    }

    return std::bit_cast<HCTextureFormatID>(format->second);
}

HCTextureFormatID hc_texture_format_id_compressed(
    HCTextureCompression compression,
    HCTextureNumericFormat numeric_format,
    HCTextureBlockSize block_size
) {
    auto const format = hc::render::COMPRESSED_FORMAT_MAP.find({compression, numeric_format, block_size});
    if (format == hc::render::COMPRESSED_FORMAT_MAP.end()) {
        return std::numeric_limits<HCTextureFormatID>::max();
    }

    return std::bit_cast<HCTextureFormatID>(format->second);
}

HCResult hc_create_texture(
    HCTexture* texture,
    u32 device,
    HCTextureDimensions dims,
    HCTextureFormatID format,
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

    image_info.format = std::bit_cast<VkFormat>(format);

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
    auto device_res = hc::render::device_at(device_id);
    if (!device_res) {
        return;
    }

    (*device_res)->destroy_texture(texture->id);
    *texture = hc::render::INVALID_TEXTURE;
}
