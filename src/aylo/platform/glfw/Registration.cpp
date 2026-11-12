/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Registration
*/

#include "Registration.hpp"

#include <memory>

#include "GlfwPlatform.hpp"
#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "core/config/Settings.hpp"

namespace aylo::platform::glfw {

void registerPlatform(contract::platform::PlatformRegistry& registry) {
    registry.add("glfw", [](const core::config::Settings& settings) -> std::unique_ptr<contract::platform::IPlatform> {
        settings.checkKeys({}, "glfw");
        return std::make_unique<GlfwPlatform>();
    });
}

}  // namespace aylo::platform::glfw
