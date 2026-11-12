/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Semaphore
*/

#include "Semaphore.hpp"

#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace aylo::render::vulkan::sync {

Semaphore::Semaphore(const core::Device& device) : _semaphore{device.getDevice(), vk::SemaphoreCreateInfo{}} {}

}  // namespace aylo::render::vulkan::sync
