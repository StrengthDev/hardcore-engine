#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>

/**
 * @brief Error codes returned by some **Hardcore** functions.
 */
enum HCError {
    HCError_GLFWInitFailed, //!< Failed to initialize global GLFW context.
    HCError_GLFWAlreadyInitialized, //!< GLFW context has already been initialized.
    HCError_VulkanInitFailed, //!< Failed to initialize global Vulkan context.
    HCError_VulkanLayerNotFound, //!< One or more required Vulkan layers were not found.
    HCError_VulkanExtensionNotFound, //!< One or more required Vulkan extensions were not found.
    HCError_VulkanFeatureNotSupported, //!< One or more required Vulkan features are not supported by the hardware.
    HCError_OutOfHostMemory, //!< Out of host memory.
    HCError_OutOfDeviceMemory, //!< Out of device memory.
    HCError_IncompatibleDriver, //!< System driver is not compatible with the Vulkan version being used.
    HCError_NoDevices, //!< No devices were found.
    HCError_NoSuchDevice, //!< The specified device does not exist.
    HCError_InvalidParams, //!< The provided parameters are not valid.
    HCError_GLFWWindowCreationFailure, //!< Failed to create new GLFW window.
    HCError_NativeWindowInUse, //!< The native window is already being used by another API.
    HCError_NoPresentSupport, //!< Presentation of the surface is not supported by the specified device.
    HCError_DeviceLost, //!< The specified device is no longer valid.
    HCError_SurfaceLost, //!< The specified surface is no longer valid.
    HCError_CompressionExhausted, //!< Internal resources required for compression are exhausted.
    HCError_InvalidOpaqueCaptureAddress, //!< Invalid opaque capture address.
    HCError_InvalidExternalHandle, //!< An external handle is not a valid handle of the specified type.
    HCError_FormatNotSupported, //!< The provided format is not supported by the hardware.
    HCError_TextureParamsNotSupported, //!< The provided texture parameters are not supported by the hardware for the given format.
    HCError_CouldNotFitInPool, //!< Could not allocate the desired range in the specified memory pool.
    HCError_UnmetHeapRequirements, //!< Hardware does not support required heap capabilities.
    HCError_MemoryMapFailed, //!< Failed to map device memory.
    HCError_UnmetQueueRequirements, //!< Hardware does not support required queue capabilities.
    HCError_InvalidRenderGraph, //!< Attempted to use the render graph while it is in an invalid state.
    HCError_ShaderReflectionFailed, //!< Failed to perform shader resource reflection.
    HCError_DescriptorMismatch, //!< Two or more descriptors did not match.
    HCError_VulkanUnknown, //!< Unknown Vulkan error, could be invalid input or an implementation failure.
    HCError_ValidationFailed, //!< Command failed due to invalid usage.
    HCError_Fragmentation, //!< Could not create descriptor pool due to fragmentation.
    HCError_InvalidShader, //!< Invalid shader.
    HCError_NoSuchResource, //!< The provided resource input does not exist.
    HCError_NoSuchNode, //!< The provided execution graph node does not exist.
    HCError_PrunedNode, //!< The provided execution graph node has deleted dependencies.
};

/**
 * @brief The result of some operation.
 */
struct HCResult {
    enum HCError error; //!< An error code, only relevant if the operation was not successful.
    bool success; //!< Indicates if an operation was successful.
};

#ifdef __cplusplus
}
#endif // __cplusplus
