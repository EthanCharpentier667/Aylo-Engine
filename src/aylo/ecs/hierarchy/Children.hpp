/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Children
*/

#pragma once

#include <vector>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"

namespace aylo::ecs {
/// @brief This entity's children, in the order they were attached. Kept by the World: read it, never write it.
/// Not replicated: each side rebuilds it from Parent.
struct Children {
    std::vector<Entity> entities;  ///< The children.
};
AYLO_ECS_COMPONENT(Children, "rtype.Children", entities);
}  // namespace aylo::ecs
