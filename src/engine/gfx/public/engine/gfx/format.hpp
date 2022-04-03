#pragma once

#include <string>

namespace ze::gfx
{

enum class Format
{
    Undefined,

    /** Depth 16-bit (unsigned short normalized) */
    D16Unorm,

    /** Depth 32-bit (signed float) */
    D32Sfloat,

    /** Depth 32-bit (signed float) & stencil 8-bit (unsigned int) */
    D32SfloatS8Uint,

    /** Depth 24-bit (signed float) & stencil 8-bit (unsigned int) */
    D24UnormS8Uint,

    /** R 8-bit (unsigned short) */
    R8Unorm,

    /** RGB 8-bit (unsigned short) */
    R8G8B8Unorm,

    /** RGBA 8-bit (unsigned short) */
    R8G8B8A8Unorm,

    /** RGB 8-bit (srgb nonlinear) */
    R8G8B8A8Srgb,

    /** BGRA 8-bit (unsigned short) */
    B8G8R8A8Unorm,

    /** RGBA 16-bit (signed float) */
    R16G16B16A16Sfloat,

    /** RG 32-bit (signed float) */
    R32G32Sfloat,

    /** RGB 32-bit (signed float) */
    R32G32B32Sfloat,

    /** RGBA 32-bit (signed float) */
    R32G32B32A32Sfloat,

    /** RGBA 32-bit (unsigned int) */
    R32G32B32A32Uint,

    /** R 32-bit (unsigned int) */
    R32Uint,

    /** R 64-bit (unsigned int) */
    R64Uint,

    /** Exotic formats */
    R10G10B10A2Unorm,

    /** BC1/DXT1 */
    Bc1RgbUnormBlock,
    Bc1RgbaUnormBlock,
    Bc1RgbSrgbBlock,
    Bc1RgbaSrgbBlock,

    /** BC3/DXT5 */
    Bc3UnormBlock,
    Bc3SrgbBlock,

    /** BC5 */
    Bc5UnormBlock,
    Bc5SnormBlock,

    /** BC6H */
    Bc6HUfloatBlock,
    Bc6HSfloatBlock,

    /** BC7 */
    Bc7UnormBlock,
    Bc7SrgbBlock,
};

inline std::string_view to_string(const Format in_format)
{
    switch(in_format)
    {
        case Format::Undefined:
            return "Undefined";
        case Format::D24UnormS8Uint:
            return "D24UnormS8Uint";
        case Format::D32Sfloat:
            return "D32Sfloat";
        case Format::D32SfloatS8Uint:
            return "D32SfloatS8Uint";
        case Format::B8G8R8A8Unorm:
            return "B8G8R8A8Unorm";
        case Format::R8G8B8Unorm:
            return "R8G8B8Unorm";
        case Format::R8G8B8A8Unorm:
            return "R8G8B8A8Unorm";
        case Format::R8G8B8A8Srgb:
            return "R8G8B8A8Srgb";
        case Format::R16G16B16A16Sfloat:
            return "R16G16B16A16Sfloat";
        case Format::R32Uint:
            return "R32Uint";
        case Format::R64Uint:
            return "R64Uint";
        case Format::R32G32Sfloat:
            return "R32G32Sfloat";
        case Format::R32G32B32Sfloat:
            return "R32G32B32Sfloat";
        case Format::R32G32B32A32Sfloat:
            return "R32G32B32A32Sfloat";
        case Format::R32G32B32A32Uint:
            return "R32G32B32A32Uint";
        case Format::Bc1RgbUnormBlock:
            return "Bc1RgbUnormBlock";
        case Format::Bc1RgbaUnormBlock:
            return "Bc1RgbaUnormBlock";
        case Format::Bc1RgbSrgbBlock:
            return "Bc1RgbSrgbBlock";
        case Format::Bc1RgbaSrgbBlock:
            return "Bc1RgbaSrgbBlock";
        case Format::Bc3UnormBlock:
            return "Bc3UnormBlock";
        case Format::Bc3SrgbBlock:
            return "Bc3SrgbBlock";
        case Format::Bc7UnormBlock:
            return "Bc7UnormBlock";
        case Format::Bc7SrgbBlock:
            return "Bc7SrgbBlock";
        case Format::Bc5UnormBlock:
            return "Bc5UnormBlock";
        case Format::Bc5SnormBlock:
            return "Bc5SnormBlock";
        case Format::Bc6HUfloatBlock:
            return "Bc6HUfloatBlock";
        case Format::Bc6HSfloatBlock:
            return "Bc6HSfloatBlock";
        default:
            ZE_UNREACHABLE();
    }
}
}