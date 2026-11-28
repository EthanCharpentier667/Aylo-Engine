/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BehaviourCatalog
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "editor/Export.hpp"

namespace aylo::editor::behaviour {
/// @brief A capsule node that carries a Behaviour: spawning the capsule runs the Behaviour's hooks on that node.
struct BehaviourCarrier {
    std::string capsule;    ///< The capsule's path, `actors/planet_big.actor`.
    std::uint32_t node{0};  ///< The node's index in the capsule; 0 is the root.
    std::string nodePath;   ///< The node's path, `.` for the root, `Boss/LeftTurret`.
};

/// @brief What the editor's Behaviours panel shows of one Behaviour: where its code is, and where it runs.
struct BehaviourEntry {
    std::string name;                     ///< Its component, `game.Spinner`.
    std::vector<std::string_view> hooks;  ///< The hooks it defines (hooksOf()).
    std::optional<std::filesystem::path>
        file;                                ///< The file of `scripts/behaviours/` that declares it; none: not found.
    std::vector<BehaviourCarrier> carriers;  ///< The capsule nodes that carry it, by capsule path then node.
    std::size_t live{0};                     ///< The entities of the world that have it now.
};

/// @return Every Behaviour of @p app (isBehaviour()), by name: its hooks, its file in the `scripts/behaviours/` of the
/// game whose assets are @p assets, the capsule nodes that carry it (the `Capsules` resource, when the App has one) and
/// how many entities of the world have it.
[[nodiscard]] AYLO_EDITOR_API std::vector<BehaviourEntry> catalogBehaviours(const ecs::App& app,
                                                                            const std::filesystem::path& assets);
}  // namespace aylo::editor::behaviour
