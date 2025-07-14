#include <pch.hpp>

#include "error.hpp"

#include <util/flow.hpp>

namespace hc {
    Error::Error(VkResult result) {
        switch (result) {
        case VK_ERROR_INITIALIZATION_FAILED:
            this->error = HCError_VulkanInitFailed;
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            this->error = HCError_OutOfHostMemory;
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            this->error = HCError_OutOfDeviceMemory;
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            this->error = HCError_VulkanLayerNotFound;
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            this->error = HCError_VulkanExtensionNotFound;
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            this->error = HCError_IncompatibleDriver;
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            this->error = HCError_NativeWindowInUse;
            break;
        case VK_ERROR_DEVICE_LOST:
            this->error = HCError_DeviceLost;
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            this->error = HCError_SurfaceLost;
            break;
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            this->error = HCError_CompressionExhausted;
            break;
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
            this->error = HCError_InvalidOpaqueCaptureAddress;
            break;
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            this->error = HCError_InvalidExternalHandle;
            break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            this->error = HCError_FormatNotSupported;
            break;
        case VK_ERROR_MEMORY_MAP_FAILED:
            this->error = HCError_MemoryMapFailed;
            break;
        default: HC_UNREACHABLE("All relevant return types must be implemented");
        }
    }
}
