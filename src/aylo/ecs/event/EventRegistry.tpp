/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventRegistry
*/

namespace aylo::ecs {
template <Event T>
EventInfo KindTraits<EventInfo>::make(const EventOptions& options) {
    EventInfo info = EventTraits<T>::makeInfo();
    info.type.presence = options.presence;
    return info;
}
}  // namespace aylo::ecs
