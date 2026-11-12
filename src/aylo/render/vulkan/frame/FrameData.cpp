/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FrameData
*/

#include "FrameData.hpp"

#include "render/vulkan/core/Device.hpp"

namespace aylo::render::vulkan::frame {

FrameData::FrameData(const core::Device& device)
    : _commandPool{device, device.getGraphicsQueue().family},
      _commandBuffer{_commandPool.allocate()},
      _imageAvailable{device},
      _inFlight{device, true} {}

}  // namespace aylo::render::vulkan::frame
