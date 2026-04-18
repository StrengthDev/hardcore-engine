
#pragma once

#include "util.hpp"

#include <core/error.hpp>
#include <core/log.hpp>

#include <util/flow.hpp>
#include <util/number.hpp>

#include <vulkan/vulkan.h>
#include <volk.h>

#include <expected>
#include <vector>

namespace vk {
    template<typename T>
    class HandleBase {
    public:
        ~HandleBase() {
            HC_ASSERT(this->value == VK_NULL_HANDLE, << typeid(T).name() << " must be manually destroyed");
        }

        HandleBase(const HandleBase&) = delete;

        HandleBase& operator=(const HandleBase&) = delete;

        HandleBase(HandleBase&& other) noexcept : value(std::exchange(other.value, VK_NULL_HANDLE)) {}

        HandleBase& operator=(HandleBase&& other) noexcept {
            HC_ASSERT(!this->valid(), "Inner value cannot be overwritten if already assigned");
            this->value = std::exchange(other.value, VK_NULL_HANDLE);
            return *this;
        }

        operator T() const noexcept { return this->value; }

        [[nodiscard]] T const& get() const noexcept { return this->value; }

        [[nodiscard]] bool valid() const noexcept { return this->value != VK_NULL_HANDLE; }

    protected:
        HandleBase() = default;

        T value = VK_NULL_HANDLE;
    };

    template<typename T>
    class HandlesBase {
    public:
        ~HandlesBase() {
            HC_ASSERT(!this->valid(), << typeid(T).name() << " must be manually destroyed");
        }

        HandlesBase(const HandlesBase&) = delete;

        HandlesBase& operator=(const HandlesBase&) = delete;

        HandlesBase(HandlesBase&& other) noexcept : values(std::move(other.values)) {}

        HandlesBase& operator=(HandlesBase&& other) noexcept {
            HC_ASSERT(!this->valid(), "Inner values cannot be overwritten if already assigned");
            this->values = std::move(other.values);
            return *this;
        }

        [[nodiscard]] Sz size() const noexcept {
            return this->values.size();
        }

        [[nodiscard]] bool valid() const noexcept {
            return !this->values.empty() && this->values.back() != VK_NULL_HANDLE;
        }

        [[nodiscard]] T const& operator[](Sz index) const {
            return this->values[index];
        }

        [[nodiscard]] T const* ptr() const noexcept {
            return this->values.data();
        }

    protected:
        HandlesBase() : HandlesBase(0) {}

        explicit HandlesBase(Sz count)
            : values(count) {
            for (auto& value : this->values) {
                value = VK_NULL_HANDLE;
            }
        }

        std::vector<T> values = VK_NULL_HANDLE;
    };

    template<const unsigned I, typename... Args>
    struct ArgGetter;

    template<typename Arg, typename... Args>
    struct ArgGetter<0, Arg, Args...> {
        using Type = Arg;
    };

    template<const unsigned I, typename Arg, typename... Args>
    struct ArgGetter<I, Arg, Args...> {
        using Type = ArgGetter<I - 1, Args...>::Type;
    };

    template<typename T>
    struct Signature;

    template<typename R, typename... Args>
    struct Signature<R(**)(Args...)> {
        template<const unsigned I>
        using Arg = ArgGetter<I, Args...>::Type;
    };

    template<typename R, typename... Args>
    struct Signature<R(*VolkDeviceTable::*)(Args...)> {
        template<const unsigned I>
        using Arg = ArgGetter<I, Args...>::Type;
    };

    class Instance : public HandleBase<VkInstance> {
    public:
        Instance() = default;

        [[nodiscard]]
        static std::expected<Instance, hc::Error> create(VkInstanceCreateInfo const* create_info) {
            VkInstance vulkan_handle = VK_NULL_HANDLE;
            VkResult const result = vkCreateInstance(create_info, nullptr, &vulkan_handle);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create " << typeid(VkInstance).name() << ": " << hc::render::to_str(result));
                return hc::Error(result);
            }

            Instance handle;
            handle.value = vulkan_handle;

            return std::move(handle);
        }

        void destroy() {
            if (this->valid()) {
                vkDestroyInstance(this->value, nullptr);
                this->value = VK_NULL_HANDLE;
            }
        }
    };

    template<typename T, typename CT, CT create_fn, typename DT, DT destroy_fn>
    class InstanceHandle : public HandleBase<T> {
    public:
        using CreateInfo = Signature<CT>::template Arg<1>;

        InstanceHandle() = default;

        [[nodiscard]]
        static std::expected<InstanceHandle, hc::Error> create(VkInstance instance, CreateInfo create_info) {
            T vulkan_handle = VK_NULL_HANDLE;
            VkResult const result = (*create_fn)(instance, create_info, nullptr, &vulkan_handle);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create " << typeid(T).name() << ": " << hc::render::to_str(result));
                return hc::Error(result);
            }

            InstanceHandle handle;
            handle.value = vulkan_handle;

            return std::move(handle);
        }

        void destroy(VkInstance instance) {
            if (this->valid()) {
                (*destroy_fn)(instance, this->value, nullptr);
                this->value = VK_NULL_HANDLE;
            }
        }
    };

#define STANDARD_VK_INSTANCE_HANDLE(Handle, CreateFn, DestroyFn) InstanceHandle<Handle, decltype(&CreateFn), &CreateFn, decltype(&DestroyFn), &DestroyFn>

    typedef STANDARD_VK_INSTANCE_HANDLE(VkDebugUtilsMessengerEXT, vkCreateDebugUtilsMessengerEXT, vkDestroyDebugUtilsMessengerEXT) DebugUtilsMessenger;

#undef STANDARD_VK_INSTANCE_HANDLE

    class Device : public HandleBase<VkDevice> {
    public:
        Device() = default;

        [[nodiscard]]
        static std::expected<Device, hc::Error> create(VkPhysicalDevice device, VkDeviceCreateInfo const* create_info) {
            VkDevice vulkan_handle = VK_NULL_HANDLE;
            VkResult const result = vkCreateDevice(device, create_info, nullptr, &vulkan_handle);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create " << typeid(VkDevice).name() << ": " << hc::render::to_str(result));
                return hc::Error(result);
            }

            Device handle;
            handle.value = vulkan_handle;

            return std::move(handle);
        }

        void destroy(VolkDeviceTable const& fn_table) {
            if (this->valid()) {
                fn_table.vkDestroyDevice(this->value, nullptr);
                this->value = VK_NULL_HANDLE;
            }
        }
    };

    template<typename T, typename CT, CT create_fn, typename DT, DT destroy_fn>
    class DeviceHandle : public HandleBase<T> {
    public:
        using CreateInfo = Signature<CT>::template Arg<1>;

        DeviceHandle() = default;

        [[nodiscard]]
        static std::expected<DeviceHandle, hc::Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            CreateInfo create_info
        ) {
            T vulkan_handle = VK_NULL_HANDLE;
            VkResult const result = (fn_table.*create_fn)(device, create_info, nullptr, &vulkan_handle);
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to create " << typeid(T).name() << ": " << hc::render::to_str(result));
                return hc::Error(result);
            }

            DeviceHandle handle;
            handle.value = vulkan_handle;

            return std::move(handle);
        }

        void destroy(VolkDeviceTable const& fn_table, VkDevice device) {
            if (this->valid()) {
                (fn_table.*destroy_fn)(device, this->value, nullptr);
                this->value = VK_NULL_HANDLE;
            }
        }
    };

#define STANDARD_VK_DEVICE_HANDLE(Handle, CreateFn, DestroyFn) DeviceHandle<Handle, decltype(&VolkDeviceTable::CreateFn), &VolkDeviceTable::CreateFn, decltype(&VolkDeviceTable::DestroyFn), &VolkDeviceTable::DestroyFn>

    typedef STANDARD_VK_DEVICE_HANDLE(VkPipelineCache, vkCreatePipelineCache, vkDestroyPipelineCache) PipelineCache;
    typedef STANDARD_VK_DEVICE_HANDLE(VkRenderPass, vkCreateRenderPass, vkDestroyRenderPass) RenderPass;
    typedef STANDARD_VK_DEVICE_HANDLE(VkImage, vkCreateImage, vkDestroyImage) Image;
    typedef STANDARD_VK_DEVICE_HANDLE(VkImageView, vkCreateImageView, vkDestroyImageView) ImageView;
    typedef STANDARD_VK_DEVICE_HANDLE(VkSwapchainKHR, vkCreateSwapchainKHR, vkDestroySwapchainKHR) Swapchain;
    typedef STANDARD_VK_DEVICE_HANDLE(VkFramebuffer, vkCreateFramebuffer, vkDestroyFramebuffer) Framebuffer;
    typedef STANDARD_VK_DEVICE_HANDLE(VkCommandPool, vkCreateCommandPool, vkDestroyCommandPool) CommandPool;
    typedef STANDARD_VK_DEVICE_HANDLE(VkFence, vkCreateFence, vkDestroyFence) Fence;
    typedef STANDARD_VK_DEVICE_HANDLE(VkSemaphore, vkCreateSemaphore, vkDestroySemaphore) Semaphore;

#undef STANDARD_VK_DEVICE_HANDLE

    template<typename T, typename CT, CT create_fn, typename DT, DT destroy_fn, typename P, P count_projection>
    class DeviceHandles : public HandlesBase<T> {
    public:
        using CreateInfo = Signature<CT>::template Arg<1>;
        using Pool = Signature<DT>::template Arg<1>;

        DeviceHandles() : DeviceHandles(0) {}

        [[nodiscard]]
        static std::expected<DeviceHandles, hc::Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            CreateInfo create_info
        ) {
            DeviceHandles handles(create_info->*count_projection);

            VkResult const result = (fn_table.*create_fn)(device, create_info, handles.values.data());
            if (result != VK_SUCCESS) {
                HC_ERROR("Failed to allocate " << typeid(T).name() << ": " << hc::render::to_str(result));
                return hc::Error(result);
            }

            return std::move(handles);
        }

        void destroy(VolkDeviceTable const& fn_table, VkDevice device, Pool pool) {
            if (this->valid()) {
                (fn_table.*destroy_fn)(
                    device,
                    pool,
                    static_cast<u32>(this->values.size()),
                    this->values.data()
                );

                for (auto& value : this->values) {
                    value = VK_NULL_HANDLE;
                }
            }
        }

    private:
        explicit DeviceHandles(Sz count) : HandlesBase<T>(count) {}
    };

#define STANDARD_VK_DEVICE_HANDLES(Handle, CreateFn, DestroyFn, CountPredicate) DeviceHandles<Handle, decltype(&VolkDeviceTable::CreateFn), &VolkDeviceTable::CreateFn, decltype(&VolkDeviceTable::DestroyFn), &VolkDeviceTable::DestroyFn, decltype(&CountPredicate), &CountPredicate>

    typedef STANDARD_VK_DEVICE_HANDLES(VkCommandBuffer, vkAllocateCommandBuffers, vkFreeCommandBuffers, VkCommandBufferAllocateInfo::commandBufferCount) CommandBuffers;

#undef STANDARD_VK_DEVICE_HANDLES
}
