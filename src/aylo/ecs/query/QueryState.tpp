/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryState
*/

#include <array>
#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/query/QueryMatcher.hpp"

namespace aylo::ecs {
template <typename... Terms>
    requires ValidQuery<Terms...>
QueryState<Terms...>::Iteration::Iterator::Iterator(Iteration& iteration) noexcept : _iteration{&iteration} {}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename QueryState<Terms...>::Item QueryState<Terms...>::Iteration::Iterator::operator*() const noexcept {
    return _iteration->makeItem(std::index_sequence_for<Terms...>{});
}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename QueryState<Terms...>::Iteration::Iterator& QueryState<Terms...>::Iteration::Iterator::operator++() noexcept {
    _iteration->_cursor.advance();
    return *this;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
void QueryState<Terms...>::Iteration::Iterator::operator++(int) noexcept {
    ++*this;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
bool QueryState<Terms...>::Iteration::Iterator::operator==(std::default_sentinel_t /*sentinel*/) const noexcept {
    return _iteration->_cursor.isDone();
}

template <typename... Terms>
    requires ValidQuery<Terms...>
QueryState<Terms...>::Iteration::Iteration(const DynamicQuery& query, Tick lastRun, Tick thisRun) noexcept
    : _cursor{query.getTerms(), resolvePools(query), {}, lastRun, thisRun} {}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename QueryState<Terms...>::Iteration::Iterator QueryState<Terms...>::Iteration::begin() noexcept {
    return Iterator{*this};
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::default_sentinel_t QueryState<Terms...>::Iteration::end() noexcept {
    return std::default_sentinel;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::array<storage::SparseSetStorage*, sizeof...(Terms)> QueryState<Terms...>::Iteration::resolvePools(
    const DynamicQuery& query) noexcept {
    std::array<storage::SparseSetStorage*, sizeof...(Terms)> pools{};
    query.resolvePools(pools);
    return pools;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
template <std::size_t... Indices>
typename QueryState<Terms...>::Item QueryState<Terms...>::Iteration::makeItem(
    std::index_sequence<Indices...> indices) const noexcept {
    return QueryState::makeItem(_cursor.getEntity(), _cursor.getPools(), _cursor.getPositions(), _cursor.getThisRun(),
                                indices);
}

template <typename... Terms>
    requires ValidQuery<Terms...>
template <std::size_t... Indices>
typename QueryState<Terms...>::Item QueryState<Terms...>::makeItem(
    const Entity entity, const Pools& pools, const Positions& positions, const Tick thisRun,
    std::index_sequence<Indices...> /*indices*/) noexcept {
    return std::tuple_cat(std::tuple<Entity>{entity}, fetch<Indices>(pools, positions, thisRun)...);
}

template <typename... Terms>
    requires ValidQuery<Terms...>
template <std::size_t Index>
auto QueryState<Terms...>::fetch(const Pools& pools, const Positions& positions, const Tick thisRun) noexcept {
    using Term = std::tuple_element_t<Index, std::tuple<Terms...>>;
    if constexpr (TermTraits<Term>::kHasItem) {
        return std::tuple<typename TermTraits<Term>::Item>{
            TermTraits<Term>::fetch(*std::get<Index>(pools), std::get<Index>(positions), thisRun)};
    } else {
        return std::tuple<>{};
    }
}

template <typename... Terms>
    requires ValidQuery<Terms...>
QueryState<Terms...>::QueryState(Components& components) : _components{&components}, _query{build(components)} {}

template <typename... Terms>
    requires ValidQuery<Terms...>
const DynamicQuery& QueryState<Terms...>::getDynamic() const noexcept {
    return _query;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::span<const ComponentAccess> QueryState<Terms...>::getAccess() const noexcept {
    return _query.getAccess();
}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename QueryState<Terms...>::Iteration QueryState<Terms...>::iter(Tick lastRun) const noexcept {
    return Iteration{_query, lastRun, _components->getChangeTick()};
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::optional<typename QueryState<Terms...>::Item> QueryState<Terms...>::get(const Entity entity,
                                                                             const Tick lastRun) const noexcept {
    Pools pools{};
    _query.resolvePools(pools);
    Positions positions{};
    const Tick thisRun = _components->getChangeTick();
    // No driver term (its index is past the last one): every term's pool is searched for the entity.
    if (!detail::matchEntity(_query.getTerms(), entity, sizeof...(Terms), 0, pools, positions, lastRun, thisRun)) {
        return std::nullopt;
    }
    return makeItem(entity, pools, positions, thisRun, std::index_sequence_for<Terms...>{});
}

template <typename... Terms>
    requires ValidQuery<Terms...>
DynamicQuery QueryState<Terms...>::build(Components& components) {
    std::vector<QueryTerm> terms{
        QueryTerm{.id = idOf<typename TermTraits<Terms>::Type>(components), .kind = TermTraits<Terms>::kKind}...};
    // Cannot fail: ValidQuery checked the terms at compile time, and every component is now registered.
    return std::move(DynamicQuery::create(components, std::move(terms))).value();
}

template <typename... Terms>
    requires ValidQuery<Terms...>
template <Component T>
ComponentId QueryState<Terms...>::idOf(Components& components) {
    if (const ComponentInfo* info = components.getRegistry().find<T>()) {
        return info->type.id;  // keeps the options it was registered with
    }
    return components.registerType<T>();
}
}  // namespace aylo::ecs
