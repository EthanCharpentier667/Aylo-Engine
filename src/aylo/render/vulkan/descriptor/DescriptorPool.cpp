/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DescriptorPool
*/

#include "DescriptorPool.hpp"

#include <cstdint>
#include <span>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include "DescriptorSetLayout.hpp"
#include "render/vulkan/core/Device.hpp"

namespace aylo::render::vulkan::descriptor {

DescriptorPool::DescriptorPool(const core::Device& device, std::uint32_t maxSets,
                               std::span<const vk::DescriptorPoolSize> sizes)
    : _device{&device},
      _pool{device.getDevice(), vk::DescriptorPoolCreateInfo{}
                                    .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                                    .setMaxSets(maxSets)
                                    .setPoolSizes(sizes)} {}

vk::raii::DescriptorSet DescriptorPool::allocate(const DescriptorSetLayout& layout) {
    vk::raii::DescriptorSets sets{
        _device->getDevice(),
        vk::DescriptorSetAllocateInfo{}.setDescriptorPool(*_pool).setSetLayouts(*layout.getLayout())};
    return std::move(sets.front());
}

}  // namespace aylo::render::vulkan::descriptor
