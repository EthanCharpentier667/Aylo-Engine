/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Registration
*/

#pragma once

#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "aylo/platform/sdl/Export.hpp"

namespace aylo::platform::sdl {

/// @brief Registers the platforms of this module: "sdl" (SdlPlatform, no setting of its own).
AYLO_PLATFORM_SDL_API void registerPlatform(contract::platform::PlatformRegistry& registry);

}  // namespace aylo::platform::sdl
