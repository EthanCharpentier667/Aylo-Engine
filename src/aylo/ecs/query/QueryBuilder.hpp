/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryBuilder
*/

#pragma once

#include <vector>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/query/DynamicQuery.hpp"
#include "aylo/ecs/query/QueryTerm.hpp"

namespace aylo::ecs {
/// @brief Collects the terms of a DynamicQuery, then checks and builds it.
/// @code
/// aylo::ecs::Result<aylo::ecs::DynamicQuery> query = aylo::ecs::QueryBuilder{}
///     .write(shieldId)
///     .read(positionId)
///     .without(deadId)
///     .build(world.getComponents());
/// @endcode
class AYLO_ECS_API QueryBuilder {
  public:
    QueryBuilder() = default;
    ~QueryBuilder() = default;
    QueryBuilder(const QueryBuilder& other) = default;
    QueryBuilder& operator=(const QueryBuilder& other) = default;
    QueryBuilder(QueryBuilder&& other) noexcept = default;
    QueryBuilder& operator=(QueryBuilder&& other) noexcept = default;

    /// @brief Requires the component and reads its value.
    QueryBuilder& read(ComponentId id);

    /// @brief Requires the component and writes its value.
    QueryBuilder& write(ComponentId id);

    /// @brief Reads the component's value when the entity has it, without requiring it.
    QueryBuilder& optionalRead(ComponentId id);

    /// @brief Writes the component's value when the entity has it, without requiring it.
    QueryBuilder& optionalWrite(ComponentId id);

    /// @brief Requires the component, without accessing its value.
    QueryBuilder& with(ComponentId id);

    /// @brief Skips the entities that have the component.
    QueryBuilder& without(ComponentId id);

    /// @brief Requires the component, added since the query last ran.
    QueryBuilder& added(ComponentId id);

    /// @brief Requires the component, added or written since the query last ran.
    QueryBuilder& changed(ComponentId id);

    /// @brief Checks the terms and builds the query, as DynamicQuery::create() does.
    /// @param components The components of the world the query iterates; they must outlive the query.
    /// @return The query, or an ErrorKind::kInvalidQuery error naming the faulty component.
    [[nodiscard]] Result<DynamicQuery> build(Components& components) const;

  private:
    /// @brief Appends a term.
    QueryBuilder& add(ComponentId id, TermKind kind);

    std::vector<QueryTerm> _terms;  ///< The terms, in the order given.
};
}  // namespace aylo::ecs
