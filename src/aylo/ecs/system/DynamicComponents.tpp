/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicComponents
*/

#include <span>
#include <type_traits>

#include "aylo/ecs/storage/ViewAs.hpp"

namespace aylo::ecs {
template <Component T>
const T* DynamicComponents::get(const Entity entity, const std::size_t index) const noexcept {
    static_assert(!std::is_empty_v<T>, "a tag has no value: test it with has()");
    const ComponentInfo* info = _context.world->getComponents().getRegistry().find<T>();
    if (info == nullptr || info->type.id != getUse(index).id) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = get(entity, index);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}
}  // namespace aylo::ecs
