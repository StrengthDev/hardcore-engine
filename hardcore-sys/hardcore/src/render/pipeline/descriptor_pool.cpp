#include <pch.hpp>

#include "descriptor_pool.hpp"

#include "../util.hpp"

#include <util/static_map.hpp>

namespace hc::render {
    static u32 constexpr DEFAULT_SET_CAPACITY = 64;

    static StaticMap<BasicKey<DescriptorType, DescriptorType::AccelerationStructure>, VkDescriptorType> constexpr DESCRIPTOR_TYPE_MAP = {
        {DescriptorType::Sampler, VK_DESCRIPTOR_TYPE_SAMPLER},
        {DescriptorType::CombinedImageSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        {DescriptorType::SampledImage, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
        {DescriptorType::StorageImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
        {DescriptorType::UniformTexelBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER},
        {DescriptorType::StorageTexelBuffer, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER},
        {DescriptorType::UniformBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        {DescriptorType::StorageBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
        {DescriptorType::InputAttachment, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT},
        {DescriptorType::AccelerationStructure, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR},
    };

    std::expected<DescriptorSetPool, Error> DescriptorSetPool::create(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::unordered_map<u32, VkDescriptorSetLayoutBinding>&& bindings_map
    ) {
        DescriptorSetPool pool;

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(bindings_map.size());

        std::unordered_map<VkDescriptorType, VkDescriptorPoolSize> pool_sizes_map;

        for (auto const& binding : bindings_map | std::views::values) {
            bindings.push_back(binding);

            if (!pool_sizes_map.contains(binding.descriptorType)) {
                pool_sizes_map.emplace(
                    binding.descriptorType,
                    VkDescriptorPoolSize{.type = binding.descriptorType, .descriptorCount = 0}
                );
            }

            pool_sizes_map[binding.descriptorType].descriptorCount++;
        }

        // TODO have dedicated layout functionality to handle dynamic binding counts (metadata/specialisation)
        VkDescriptorSetLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<u32>(bindings.size()),
            .pBindings = bindings.data(),
        };

        // TODO result
        fn_table.vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &pool.layout.get());

        pool.pool_size_bases.reserve(pool_sizes_map.size());
        for (auto const& pool_size : pool_sizes_map | std::views::values) {
            pool.pool_size_bases.push_back(pool_size);
        }

        pool.capacity = DEFAULT_SET_CAPACITY;

        std::vector<VkDescriptorPoolSize> pool_sizes(pool.pool_size_bases);
        for (auto& pool_size : pool_sizes) {
            pool_size.descriptorCount *= DEFAULT_SET_CAPACITY;
        }

        VkDescriptorPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = DEFAULT_SET_CAPACITY,
            .poolSizeCount = static_cast<u32>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };

        VkResult result = fn_table.vkCreateDescriptorPool(device, &pool_info, nullptr, &pool.handle.get());
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to create descriptor pool: " << to_str(result));
            return Error(result);
        }

        return pool;
    }

    void DescriptorSetPool::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        if (this->handle.valid()) {
            fn_table.vkDestroyDescriptorPool(device, this->handle, nullptr);
            this->handle.destroy();
        }
    }

    std::expected<DescriptorPool, Error> DescriptorPool::create(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        std::vector<Shader> const& shaders
    ) {
        DescriptorPool pool;

        std::unordered_map<DescriptorLocation, DescriptorBinding> descriptor_map;
        std::unordered_map<u32, std::unordered_map<u32, VkDescriptorSetLayoutBinding>> sets_map;

        for (auto const& shader : shaders) {
            for (auto const& [location, binding] : shader.bindings()) {
                if (descriptor_map.contains(location)) {
                    if (descriptor_map[location] != binding) {
                        HC_ERROR("Shaders have conflicting bindings at: set=" << location.set << " binding=" << location.binding);
                        return Error(HCError_DescriptorMismatch);
                    }
                } else {
                    descriptor_map.emplace(location, binding);
                }

                VkDescriptorType const* descriptor_type = DESCRIPTOR_TYPE_MAP[binding.type];
                HC_ASSERT(descriptor_type, "Descriptor type must exist");

                auto& bindings_map = sets_map[location.set];
                if (!bindings_map.contains(location.binding)) {
                    bindings_map.emplace(
                        location.binding,
                        VkDescriptorSetLayoutBinding{
                            .binding = location.binding,
                            .descriptorType = *descriptor_type,
                            // TODO extract from binding, if 0 needs to be specialised
                            .descriptorCount = 1,
                            .stageFlags = 0,
                            .pImmutableSamplers = nullptr
                        }
                    );
                }

                bindings_map[*descriptor_type].stageFlags |= shader.stage_flag();
            }
        }

        pool.set_pools.reserve(sets_map.size());

        for (auto& [set, bindings_map] : sets_map) {
            auto set_pool_result = DescriptorSetPool::create(fn_table, device, std::move(bindings_map));
            if (!set_pool_result) {
                pool.destroy(fn_table, device);
                return set_pool_result.error();
            }

            pool.set_pools.emplace(set, *std::move(set_pool_result));
        }

        return pool;
    }

    void DescriptorPool::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        for (auto& set_pool : this->set_pools | std::views::values) {
            set_pool.destroy(fn_table, device);
        }
        this->set_pools.clear();
    }
}
