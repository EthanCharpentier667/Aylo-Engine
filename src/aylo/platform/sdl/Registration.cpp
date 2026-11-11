/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Registration
*/

#include "Registration.hpp"

#include <memory>

#include "SdlPlatform.hpp"
#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "core/config/Settings.hpp"

namespace aylo::platform::sdl {

void registerPlatform(contract::platform::PlatformRegistry& registry) {
    registry.add("sdl", [](const core::config::Settings& settings) -> std::unique_ptr<contract::platform::IPlatform> {
        settings.checkKeys({}, "sdl");
        return std::make_unique<SdlPlatform>();
    });
}

}  // namespace aylo::platform::sdl
