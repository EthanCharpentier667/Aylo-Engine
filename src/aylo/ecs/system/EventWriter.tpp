/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventWriter
*/

#include <utility>

namespace aylo::ecs {
template <Event T>
EventWriter<T>::EventWriter(Events& events) noexcept : _events{&events} {}

template <Event T>
void EventWriter<T>::send(T event) {
    _events->send(std::move(event));
}
}  // namespace aylo::ecs
