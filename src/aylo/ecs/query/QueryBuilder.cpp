/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryBuilder
*/

#include "QueryBuilder.hpp"

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/query/DynamicQuery.hpp"
#include "aylo/ecs/query/QueryTerm.hpp"

namespace aylo::ecs {
QueryBuilder& QueryBuilder::read(ComponentId id) { return add(id, TermKind::kRead); }

QueryBuilder& QueryBuilder::write(ComponentId id) { return add(id, TermKind::kWrite); }

QueryBuilder& QueryBuilder::optionalRead(ComponentId id) { return add(id, TermKind::kOptionalRead); }

QueryBuilder& QueryBuilder::optionalWrite(ComponentId id) { return add(id, TermKind::kOptionalWrite); }

QueryBuilder& QueryBuilder::with(ComponentId id) { return add(id, TermKind::kWith); }

QueryBuilder& QueryBuilder::without(ComponentId id) { return add(id, TermKind::kWithout); }

QueryBuilder& QueryBuilder::added(ComponentId id) { return add(id, TermKind::kAdded); }

QueryBuilder& QueryBuilder::changed(ComponentId id) { return add(id, TermKind::kChanged); }

Result<DynamicQuery> QueryBuilder::build(Components& components) const {
    return DynamicQuery::create(components, _terms);
}

QueryBuilder& QueryBuilder::add(ComponentId id, TermKind kind) {
    _terms.push_back(QueryTerm{.id = id, .kind = kind});
    return *this;
}
}  // namespace aylo::ecs
