/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TextureCalls
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::graphics {

inline constexpr std::string_view kLoadTextureCall = "aylo.backend.loadTexture";  ///< TextureAssets::load().

/// @brief The arguments of `aylo.backend.loadTexture`: asks for an image file.
struct LoadTextureArguments {
    ecs::String path;    ///< The image file.
    bool linear{false};  ///< Smooth sampling instead of sharp pixels.
    bool wrap{false};  ///< Tiles the texture when a source rectangle goes past its edges (`repeat` is a Luau keyword).
};
AYLO_ECS_CALL_DATA(LoadTextureArguments, "aylo.backend.LoadTextureArguments", path, linear, wrap);

/// @brief The result of `aylo.backend.loadTexture`.
struct TextureResult {
    core::asset::AssetId texture;  ///< The texture's id, for a SpriteRenderer.
};
AYLO_ECS_CALL_DATA(TextureResult, "aylo.backend.TextureResult", texture);

/// @brief Registers the texture engine call on @p app, for scripts: `aylo.backend.loadTexture`, on the
/// `aylo.TextureAssets` resource. Called by the GraphicsPlugin.
void addTextureCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addTextureForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::graphics
