/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Registration
*/

#pragma once

#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "aylo/platform/glfw/Export.hpp"

namespace aylo::platform::glfw {

/// @brief Registers the platforms of this module: "glfw" (GlfwPlatform, no setting of its own).
AYLO_PLATFORM_GLFW_API void registerPlatform(contract::platform::PlatformRegistry& registry);

}  // namespace aylo::platform::glfw
