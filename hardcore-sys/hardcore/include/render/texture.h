#pragma once

#include "../core/result.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

enum HCTextureType {
    HCTextureType_Texture1D,
    HCTextureType_Texture2D,
    HCTextureType_Texture3D,
};

/**
 * @brief The dimensions of a one-dimensional texture.
 */
struct HCTextureDimensions1D {
    uint32_t width; //!< The width of the texture.
    uint32_t layers; //!< The number of texture layers.
};

/**
 * @brief The dimensions of a two-dimensional texture.
 */
struct HCTextureDimensions2D {
    uint32_t width; //!< The width of the texture.
    uint32_t height; //!< The height of the texture.
    uint32_t layers; //!< The number of texture layers.
};

/**
 * @brief The dimensions of a three-dimensional texture.
 */
struct HCTextureDimensions3D {
    uint32_t width; //!< The width of the texture.
    uint32_t height; //!< The height of the texture.
    uint32_t depth; //!< The depth of the texture.
};

/**
 * @brief A union which may contain the dimensions of a texture with an arbitrary number of dimensions.
 */
union HCTextureDimensionsX {
    struct HCTextureDimensions1D dims1D; //!< The dimensions of a one-dimensional texture.
    struct HCTextureDimensions2D dims2D; //!< The dimensions of a two-dimensional texture.
    struct HCTextureDimensions3D dims3D; //!< The dimensions of a three-dimensional texture.
};

/**
 * @brief The dimensions of a texture.
 *
 * This should be treated as a tagged union.
 */
struct HCTextureDimensions {
    union HCTextureDimensionsX dims; //!< The underlying dimension data. The type of the data must match `texture_type`.
    enum HCTextureType texture_type; //!< The type of the texture. Must match the type of the contents of `size`.
};

/**
 * @brief The component format of a texture.
 */
enum HCTextureComponentFormat {
    HCTextureComponentFormat_R8,
    HCTextureComponentFormat_R8G8,
    HCTextureComponentFormat_R8G8B8,
    HCTextureComponentFormat_R8G8B8A8,
    HCTextureComponentFormat_R16,
    HCTextureComponentFormat_R16G16,
    HCTextureComponentFormat_R16G16B16,
    HCTextureComponentFormat_R16G16B16A16,
    HCTextureComponentFormat_R32,
    HCTextureComponentFormat_R32G32,
    HCTextureComponentFormat_R32G32B32,
    HCTextureComponentFormat_R32G32B32A32,
    HCTextureComponentFormat_R64,
    HCTextureComponentFormat_R64G64,
    HCTextureComponentFormat_R64G64B64,
    HCTextureComponentFormat_R64G64B64A64,
    HCTextureComponentFormat_A8,
    HCTextureComponentFormat_B8G8R8,
    HCTextureComponentFormat_B8G8R8A8,
};

/**
 * @brief The numeric format of each component in a texture.
 */
enum HCTextureNumericFormat {
    HCTextureNumericFormat_UNorm, //!< The components are unsigned normalized values in the range [0, 1].
    HCTextureNumericFormat_SNorm, //!< The components are signed normalized values in the range [-1, 1].
    /**
     * The components are unsigned integer values that get converted to floating-point in the range [0, 2^n - 1]
     * (n being the number of bits in the component).
     */
    HCTextureNumericFormat_UScaled,
    /**
     * The components are signed integer values that get converted to floating-point in the range
     * [-2^(n - 1), 2^(n - 1) - 1]
     * (n being the number of bits in the component).
     */
    HCTextureNumericFormat_SScaled,

    /**
     * The components are unsigned integer values in the range [0,2n - 1]
     * (n being the number of bits in the component)
     */
    HCTextureNumericFormat_UInt,
    /**
     * The components are signed integer values in the range [-2^(n - 1), 2^(n - 1) - 1]
     * [-2^(n - 1), 2^(n - 1) - 1]
     * (n being the number of bits in the component)
     */
    HCTextureNumericFormat_SInt,
    /**
     * The components are unsigned floating-point numbers (used by packed, shared exponent, and some compressed formats).
     *
     * Typically used for HDR images.
     */
    HCTextureNumericFormat_UFloat,
    /**
     * The components are signed floating-point numbers.
     *
     * Typically used for HDR images.
     */
    HCTextureNumericFormat_SFloat,
    /**
     * The R, G, and B components are unsigned normalized values that represent values using sRGB nonlinear encoding,
     * while the A component (if one exists) is a regular unsigned normalized value.
     */
    HCTextureNumericFormat_SRGB,
    /**
     * The components are signed fractional integer values that get converted to floating-point in the range
     * [-1024, 1023.96875].
     */
    // HCTextureNumericFormat_SFixed5,
};

/**
 * @brief The compression scheme used by a texture.
 */
enum HCTextureCompression {
    HCTextureCompression_BC1, //!< Block compression 1 (S3TC), encodes only RBG with no alpha.
    HCTextureCompression_BC1_A, //!< Block compression 1 (S3TC), encodes RGB with punch-through alpha.
    HCTextureCompression_BC2, //!< Block compression 2 (S3TC), encodes RGBA.
    HCTextureCompression_BC3, //!< Block compression 3 (S3TC), encodes RGBA.
    HCTextureCompression_BC4, //!< Block compression 4 (RGTC), encodes a single component channel (R).
    HCTextureCompression_BC5, //!< Block compression 5 (RGTC), encodes two component channels (RG).
    HCTextureCompression_BC6, //!< Block compression 6 (BPTC), encodes HDR data, but only RBG with no alpha.
    HCTextureCompression_BC7, //!< Block compression 7 (BPTC), encodes RGBA.
    HCTextureCompression_ETC2, //!< Ericsson Texture Compression, encodes only RGB with no alpha.
    HCTextureCompression_ETC2_A1, //!< Ericsson Texture Compression, encodes RGB with punch-through alpha.
    HCTextureCompression_ETC2_A8, //!< Ericsson Texture Compression, encodes RGBA.
    HCTextureCompression_EAC_R, //!< ETC2 Alpha Compression, encodes a single component channel (R).
    HCTextureCompression_EAC_RG, //!< ETC2 Alpha Compression, encodes two component channels (RG).
    HCTextureCompression_ASTC, //!< Adaptive Scalable Texture Compression (LDR Profile), encodes RGBA, can encode HDR data.
};

/**
 * @brief The block size used by ASTC texture compression schemes.
 */
enum HCTextureBlockSize {
    HCTextureBlockSize_BS4x4, //!< 4x4 compressed texel blocks.
    HCTextureBlockSize_BS5x4, //!< 5x4 compressed texel blocks.
    HCTextureBlockSize_BS5x5, //!< 5x5 compressed texel blocks.
    HCTextureBlockSize_BS6x5, //!< 6x5 compressed texel blocks.
    HCTextureBlockSize_BS6x6, //!< 6x6 compressed texel blocks.
    HCTextureBlockSize_BS8x5, //!< 8x5 compressed texel blocks.
    HCTextureBlockSize_BS8x6, //!< 8x6 compressed texel blocks.
    HCTextureBlockSize_BS8x8, //!< 8x8 compressed texel blocks.
    HCTextureBlockSize_BS10x5, //!< 10x5 compressed texel blocks.
    HCTextureBlockSize_BS10x6, //!< 10x6 compressed texel blocks.
    HCTextureBlockSize_BS10x8, //!< 10x8 compressed texel blocks.
    HCTextureBlockSize_BS10x10, //!< 10x10 compressed texel blocks.
    HCTextureBlockSize_BS12x10, //!< 12x10 compressed texel blocks.
    HCTextureBlockSize_BS12x12, //!< 12x12 compressed texel blocks.
};

/**
 * @brief A number representing the underlying texture format.
 *
 * This value should match exactly with the VkFormat used.
 */
typedef int32_t HCTextureFormatID;

/**
 * @brief The sample count of a texture.
 */
enum HCTextureSampleCount {
    HCTextureSampleCount_SC1, //!< Specifies a texture with one sample per pixel.
    HCTextureSampleCount_SC2, //!< Specifies a texture with 2 samples per pixel.
    HCTextureSampleCount_SC4, //!< Specifies a texture with 4 samples per pixel.
    HCTextureSampleCount_SC8, //!< Specifies a texture with 8 samples per pixel.
    HCTextureSampleCount_SC16, //!< Specifies a texture with 16 samples per pixel.
    HCTextureSampleCount_SC32, //!< Specifies a texture with 32 samples per pixel.
    HCTextureSampleCount_SC64, //!< Specifies a texture with 64 samples per pixel.
};

/**
 * @brief A device texture.
 *
 * Every field of this struct MUST be treated as const, and thus, not altered throughout its lifetime.
 *
 * This struct should never be created directly. Instead, `hc_create_texture` should be used to create a new instance,
 * and `hc_destroy_texture` used to destroy the instance.
 */
struct HCTexture {
    uint64_t id; //!< The ID of this texture within the device.
    size_t size; //!< The amount of usable memory occupied by this texture, in bytes.
    uint32_t device; //!< The ID of the device which this texture belongs to.
};

HCTextureFormatID hc_texture_format_id_standard(
    enum HCTextureComponentFormat component_format,
    enum HCTextureNumericFormat numeric_format
);
HCTextureFormatID hc_texture_format_id_compressed(
    enum HCTextureCompression compression,
    enum HCTextureNumericFormat numeric_format,
    enum HCTextureBlockSize block_size
);

struct HCResult hc_create_texture(
    struct HCTexture* texture,
    uint32_t device,
    struct HCTextureDimensions dims,
    HCTextureFormatID format,
    uint32_t mip_levels,
    enum HCTextureSampleCount sample_count
);
void hc_destroy_texture(struct HCTexture* texture);

#ifdef __cplusplus
}
#endif // __cplusplus
