/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryList
*/

#pragma once

#include <vector>

#include "aylo/ecs/query/QueryBuilder.hpp"

namespace aylo::ecs {
/// @brief The configuration of a DynamicQueries parameter: the queries the system runs, given when it is added.
/// @details Each builder becomes one query, in the same order (DynamicQueries::iter(0) is the first). The components
/// they name must be registered before the system is added.
/// @code
/// app.addSystem(aylo::ecs::Phase::kPostUpdate, "net.snapshot", &snapshot,
///               aylo::ecs::QueryList{{aylo::ecs::QueryBuilder{}.read(networkId).read(health)}});
/// @endcode
struct QueryList {
    std::vector<QueryBuilder> queries;  ///< One builder per query, in order.
};
}  // namespace aylo::ecs
