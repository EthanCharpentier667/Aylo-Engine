/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Name
*/

#pragma once

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/String.hpp"

namespace aylo::ecs {
/// @brief What a developer calls an entity, as the component "rtype.Name": shown by the tools (the editor's lists and
/// inspector), and found by scripts that look an entity up by name. It means nothing to the engine, which never reads
/// it: two entities may share a name, and an entity may have none.
/// @details A capsule gives each entity it spawns the name of its node (its file's name for the root), unless the node
/// gives a Name of its own. At most 255 bytes of UTF-8 (String).
struct Name {
    String value;  ///< The name.
};
AYLO_ECS_COMPONENT(Name, "rtype.Name", value);
}  // namespace aylo::ecs
