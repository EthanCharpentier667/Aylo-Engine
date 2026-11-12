/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventReader
*/

#include <cstddef>

namespace aylo::ecs {
template <Event T>
EventReader<T>::EventReader(const Events& events, EventCursor& cursor) noexcept : _events{events.read<T>(cursor)} {}

template <Event T>
std::size_t EventReader<T>::size() const noexcept {
    return _events.size();
}

template <Event T>
bool EventReader<T>::empty() const noexcept {
    return _events.empty();
}

template <Event T>
auto EventReader<T>::begin() const noexcept -> EventRange<T>::Iterator {
    return _events.begin();
}

template <Event T>
auto EventReader<T>::end() const noexcept -> EventRange<T>::Iterator {
    return _events.end();
}
}  // namespace aylo::ecs
