/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TextCalls
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::text {

inline constexpr std::string_view kLoadFontCall = "aylo.text.loadFont";  ///< FontAssets::load().

/// @brief The arguments of `aylo.text.loadFont`: reads an msdf-atlas-gen JSON file.
struct LoadFontArguments {
    ecs::String path;  ///< The JSON file, next to its atlas image.
};
AYLO_ECS_CALL_DATA(LoadFontArguments, "aylo.text.LoadFontArguments", path);

/// @brief The result of `aylo.text.loadFont`.
struct FontResult {
    core::asset::AssetId font;  ///< The font's id, for a TextRenderer.
};
AYLO_ECS_CALL_DATA(FontResult, "aylo.text.FontResult", font);

/// @brief Registers the text engine call on @p app, for scripts: `aylo.text.loadFont`, on the `aylo.FontAssets`
/// resource. Called by the TextPlugin.
void addTextCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addTextForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::text
