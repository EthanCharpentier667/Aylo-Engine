/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PipelineLayout
*/

#include "PipelineLayout.hpp"

#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace aylo::render::vulkan::pipeline {

PipelineLayout::PipelineLayout(const core::Device& device, std::span<const vk::DescriptorSetLayout> setLayouts,
                               std::span<const vk::PushConstantRange> pushConstantRanges)
    : _layout{device.getDevice(),
              vk::PipelineLayoutCreateInfo{}.setSetLayouts(setLayouts).setPushConstantRanges(pushConstantRanges)},
      _pushConstantRanges{pushConstantRanges.begin(), pushConstantRanges.end()} {}

}  // namespace aylo::render::vulkan::pipeline
