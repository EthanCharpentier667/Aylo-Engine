/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "render/vulkan/Export.hpp"

namespace aylo::render::vulkan::exceptions {

/// @brief Exception thrown when a shader cannot be loaded: missing file, or content that is not SPIR-V.
/// @details Exported so the executable can catch it when the renderer, a shared library, throws it.
class AYLO_RENDER_VULKAN_API ShaderException : public std::runtime_error {
  public:
    explicit ShaderException(const std::string& message) : std::runtime_error(message) {}
};

}  // namespace aylo::render::vulkan::exceptions
