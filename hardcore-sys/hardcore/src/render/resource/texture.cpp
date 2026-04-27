
#include <pch.hpp>

#include "texture.hpp"

#include "../renderer.hpp"
#include "../util.hpp"

#include <util/bits.hpp>
#include <util/flow.hpp>
#include <util/number.hpp>
#include <util/static_map.hpp>

bool operator==(HCTextureViewParams const& lhs, HCTextureViewParams const& rhs) {
    return lhs.base_layer == rhs.base_layer
        && lhs.layer_count == rhs.layer_count
        && lhs.base_mip_level == rhs.base_mip_level
        && lhs.mip_level_count == rhs.mip_level_count
        && lhs.cube == rhs.cube;
}

std::size_t std::hash<HCTextureViewParams>::operator()(HCTextureViewParams const& params) const noexcept {
    Sz const layer_hash = std::hash<u64>{}(concat_bits(params.base_layer, params.layer_count));
    Sz const mip_hash = std::hash<u64>{}(reverse_bits(concat_bits(params.base_mip_level, params.mip_level_count)));
    Sz const cube_hash = std::hash<bool>{}(params.cube);

    return layer_hash ^ mip_hash ^ cube_hash;
}

namespace hc::render::texture {
    static StaticMap<KeyUnion<
        BasicKey<HCTextureComponentFormat, HCTextureComponentFormat_B8G8R8A8>,
        BasicKey<HCTextureNumericFormat, HCTextureNumericFormat_SRGB>
    >, VkFormat> constexpr STANDARD_FORMAT_MAP = {
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

    static StaticMap<KeyUnion<
        BasicKey<HCTextureCompression, HCTextureCompression_ASTC>,
        BasicKey<HCTextureNumericFormat, HCTextureNumericFormat_SRGB>,
        BasicKey<HCTextureBlockSize, HCTextureBlockSize_BS12x12>
    >, VkFormat> constexpr COMPRESSED_FORMAT_MAP = {
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

    VkFormat const* to_vk_format(HCTextureFormat const& format) {
        switch (format.type) {
        case HCTextureFormatType_Standard:
            {
                auto const& [component_format, numeric_format] = format.format.standard;
                return STANDARD_FORMAT_MAP[{component_format, numeric_format}];
            }
        case HCTextureFormatType_Compressed:
            {
                auto const& [compression, numeric_format, block_size] = format.format.compressed;
                return COMPRESSED_FORMAT_MAP[{compression, numeric_format, block_size}];
            }
        }

        return nullptr;
    }

    Texture::Texture(device::memory::Ref const& ref, vk::Image&& image, VkImageCreateInfo const& image_info)
        : ref(ref), handle(std::move(image)), image_info(image_info) {}

    void Texture::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        for (auto& view : this->views | std::views::values) {
            view.handle.destroy(fn_table, device);
        }
        this->views.clear();

        this->handle.destroy(fn_table, device);
    }

    device::memory::Ref const& Texture::memory_ref() const noexcept {
        return this->ref;
    }

    VkFormat Texture::format() const noexcept {
        return this->image_info.format;
    }

    std::expected<VkImageView, Error> Texture::get_view(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        HCTextureViewParams const& params
    ) noexcept {
        if (!this->views.contains(params)) {
            auto view = create_image_view(fn_table, device, this->handle, this->image_info, params);
            if (!view) {
                return view.error();
            }

            this->views.emplace(params, TextureView{*std::move(view), 0});
        }

        TextureView& view = this->views[params];
        view.ref_count++;
        return view.handle;
    }

    void Texture::free_view(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        HCTextureViewParams const& params
    ) noexcept {
        if (!this->views.contains(params)) {
            return;
        }

        TextureView& view = this->views[params];
        view.ref_count--;

        if (view.ref_count == 0) {
            view.handle.destroy(fn_table, device);

            this->views.erase(params);
        }
    }

    std::expected<vk::Image, Error> create_image(
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
            image_info.imageType == VK_IMAGE_TYPE_3D && image_info.arrayLayers == 1,
            "3D textures must have exactly 1 layer"
        );
        HC_ASSERT(
            image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT && image_info.arrayLayers >= 6,
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

        return vk::Image::create(fn_table, device, &image_info);
    }

    std::expected<vk::ImageView, Error> create_image_view(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkImage image,
        VkImageCreateInfo const& image_info,
        HCTextureViewParams const& view_params
    ) {
        if (view_params.layer_count == 0) {
            HC_ERROR("View layer count must be greater than 0");
            return Error(HCError_InvalidParams);
        }

        if (view_params.mip_level_count == 0) {
            HC_ERROR("View mip level count must be greater than 0");
            return Error(HCError_InvalidParams);
        }

        if (image_info.arrayLayers < view_params.base_layer + view_params.layer_count) {
            HC_ERROR("View layer indexes exceed image layer count");
            return Error(HCError_InvalidParams);
        }

        if (image_info.mipLevels < view_params.base_mip_level + view_params.mip_level_count) {
            HC_ERROR("View mip level indexes exceed image mip level count");
            return Error(HCError_InvalidParams);
        }

        if (view_params.cube) {
            if (view_params.layer_count != 6) {
                HC_ERROR("Cube views must be have exactly 6 layers");
                return Error(HCError_InvalidParams);
            }

            if (image_info.imageType == VK_IMAGE_TYPE_3D) {
                if (!(image_info.flags & VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT)) {
                    HC_ERROR("3D textures must be made explicitly compatible with 2D views in order to create a cube view");
                    return Error(HCError_InvalidParams);
                }
            } else if (image_info.imageType != VK_IMAGE_TYPE_2D) {
                HC_ERROR("Cube views cannot be created for 1D textures");
                return Error(HCError_InvalidParams);
            }
        }

        VkImageViewType view_type;
        bool const is_array = view_params.cube ? view_params.layer_count / 6 > 1 : view_params.layer_count > 1;

        switch (image_info.imageType) {
        case VK_IMAGE_TYPE_1D:
            view_type = is_array ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        case VK_IMAGE_TYPE_2D:
            if (view_params.cube) {
                view_type = is_array ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
            } else {
                view_type = is_array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            }
            break;
        case VK_IMAGE_TYPE_3D:
            view_type = view_params.cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_3D;
            break;
        default: HC_UNREACHABLE("Invalid image type");
        }

        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = view_type,
            .format = image_info.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = view_params.base_mip_level,
                .levelCount = view_params.mip_level_count,
                .baseArrayLayer = view_params.base_layer,
                .layerCount = view_params.layer_count,
            },
        };

        return vk::ImageView::create(fn_table, device, &view_info);
    }
}
