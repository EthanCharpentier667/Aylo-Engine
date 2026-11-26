/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryTerms
*/

#pragma once

namespace aylo::ecs {
/// @brief Query term: the component may be absent. `Optional<T>` yields a Mut<T> that is empty when the entity
/// lacks it, `Optional<const T>` a `const T*` that is null then. Never drives the iteration.
template <typename T>
struct Optional {};

/// @brief Query filter: the entity must have the component; its value is not accessed (works for tags).
template <typename T>
struct With {};

/// @brief Query filter: the entity must not have the component.
template <typename T>
struct Without {};

/// @brief Query filter: the entity's component was added since the query last ran.
template <typename T>
struct Added {};

/// @brief Query filter: the entity's component was added or written since the query last ran.
template <typename T>
struct Changed {};

/// @brief Query filter: the entity lost the component (removed, or despawned) since the query last ran. It yields
/// nothing, and the entity may be dead.
template <typename T>
struct Removed {};

/// @brief Query filter: neither the entity nor one of its ancestors is switched off with Disabled. The systems that
/// draw, simulate or play an entity use it; a query without it sees disabled entities too.
struct Enabled {};
}  // namespace aylo::ecs
