/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** World
*/

#include <utility>

namespace aylo::ecs {
template <Component First, Component... Rest>
Entity World::spawn(First first, Rest... rest) {
    const Entity entity = spawn();
    try {
        _state->components.add(entity, std::move(first));
        (_state->components.add(entity, std::move(rest)), ...);
    } catch (...) {
        despawn(entity);  // destroys the components already added
        throw;
    }
    return entity;
}
}  // namespace aylo::ecs
