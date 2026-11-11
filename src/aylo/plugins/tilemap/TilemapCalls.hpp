/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TilemapCalls
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep

namespace aylo::plugins::tilemap {

inline constexpr std::string_view kLoadTilemapCall = "aylo.tilemap.load";    ///< TilemapAssets::load().
inline constexpr std::string_view kSpawnTilemapCall = "aylo.tilemap.spawn";  ///< spawnTilemap().

/// @brief The arguments of `aylo.tilemap.load`: reads a Tiled `.tmj` file.
struct LoadTilemapArguments {
    ecs::String path;  ///< The map file.
};
AYLO_ECS_CALL_DATA(LoadTilemapArguments, "aylo.tilemap.LoadTilemapArguments", path);

/// @brief The result of `aylo.tilemap.load`.
struct TilemapResult {
    core::asset::AssetId map;  ///< The map's id, for `aylo.tilemap.spawn`.
};
AYLO_ECS_CALL_DATA(TilemapResult, "aylo.tilemap.TilemapResult", map);

/// @brief The arguments of `aylo.tilemap.spawn`: spawns one entity per layer of a loaded map (TilemapPlacement).
struct SpawnTilemapArguments {
    core::asset::AssetId map;        ///< The map, from `aylo.tilemap.load` (or its path).
    glm::vec2 position{0.0F};        ///< The world point of the map's top-left corner.
    glm::vec2 scale{1.0F};           ///< World units per map pixel.
    std::int32_t firstDrawLayer{0};  ///< The draw layer of the map's bottom layer; each layer above adds one.
};
AYLO_ECS_CALL_DATA(SpawnTilemapArguments, "aylo.tilemap.SpawnTilemapArguments", map, position, scale, firstDrawLayer);

/// @brief The result of `aylo.tilemap.spawn`.
struct SpawnTilemapResult {
    std::uint32_t layers{0};  ///< How many layer entities were spawned; 0 if the map is unknown or failed to load.
};
AYLO_ECS_CALL_DATA(SpawnTilemapResult, "aylo.tilemap.SpawnTilemapResult", layers);

/// @brief Registers the tilemap engine calls on @p app, for scripts: `aylo.tilemap.load` and `aylo.tilemap.spawn`,
/// on the `aylo.TilemapAssets` resource. Called by the TilemapPlugin.
void addTilemapCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addTilemapForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::tilemap
