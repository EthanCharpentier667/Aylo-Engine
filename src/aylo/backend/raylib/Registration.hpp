/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Registration
*/

#pragma once

#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "aylo/contract/renderer/RendererRegistry.hpp"
#include "backend/raylib/Export.hpp"

namespace aylo::backend::raylib {

/// @brief Registers the platform "raylib" (RaylibPlatform). It reads the section "raylib" only to check its keys.
AYLO_BACKEND_RAYLIB_API void registerPlatform(contract::platform::PlatformRegistry& registry);

/// @brief Registers the renderer "raylib" (RaylibRenderer), built from the section "raylib": `virtualResolution`.
AYLO_BACKEND_RAYLIB_API void registerRenderer(contract::renderer::RendererRegistry& registry);

}  // namespace aylo::backend::raylib
