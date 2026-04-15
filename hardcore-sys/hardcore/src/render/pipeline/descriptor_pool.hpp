#pragma once

#include "../shader/shader.hpp"

#include <core/error.hpp>
#include <util/uncopyable.hpp>

#include <vulkan/vulkan.h>

#include <expected>
#include <stack>
#include <vector>

namespace hc::render {
    class DescriptorSetPool {
    public:
        DescriptorSetPool() = default;

        [[nodiscard]]
        static std::expected<DescriptorSetPool, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::unordered_map<u32, VkDescriptorSetLayoutBinding>&& bindings_map
        );

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

    private:
        ExternalHandle<VkDescriptorPool, VK_NULL_HANDLE> handle;
        ExternalHandle<VkDescriptorSetLayout, VK_NULL_HANDLE> layout;

        std::vector<VkDescriptorPoolSize> pool_size_bases;

        u32 capacity = 0;
        std::stack<ExternalHandle<VkDescriptorSet, VK_NULL_HANDLE>> unused_sets;
    };


    class DescriptorPool {
    public:
        DescriptorPool() = default;

        [[nodiscard]]
        static std::expected<DescriptorPool, Error> create(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            std::vector<Shader> const& shaders
        );

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

    private:
        std::unordered_map<u32, DescriptorSetPool> set_pools;
    };
}
