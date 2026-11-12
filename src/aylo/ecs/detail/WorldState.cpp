/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** WorldState
*/

#include "WorldState.hpp"

#include <format>
#include <string_view>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/event/EventRegistry.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/ResourceRegistry.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"

namespace aylo::ecs::detail {
namespace {
/// @return The kind at the start of a message: "Component", "Resource", "Event".
[[nodiscard]] std::string_view toLabel(TypeKind kind) noexcept {
    switch (kind) {
        case TypeKind::kComponent:
            return KindTraits<ComponentInfo>::kLabel;
        case TypeKind::kResource:
            return KindTraits<ResourceInfo>::kLabel;
        case TypeKind::kEvent:
            return KindTraits<EventInfo>::kLabel;
        case TypeKind::kCallData:
            return "Call data";
    }
    return "Type";
}

/// @return The kind inside a message, with its article: "a component", "a resource", "an event".
[[nodiscard]] std::string_view toNoun(TypeKind kind) noexcept {
    switch (kind) {
        case TypeKind::kComponent:
            return "a component";
        case TypeKind::kResource:
            return "a resource";
        case TypeKind::kEvent:
            return "an event";
        case TypeKind::kCallData:
            return "call data";
    }
    return "a type";
}
}  // namespace

Result<void> WorldState::checkNameFree(std::string_view name, TypeKind kind) const {
    TypeKind owner = kind;
    if (kind != TypeKind::kComponent && components.getRegistry().findByName(name) != nullptr) {
        owner = TypeKind::kComponent;
    } else if (kind != TypeKind::kResource && resources.getRegistry().findByName(name) != nullptr) {
        owner = TypeKind::kResource;
    } else if (kind != TypeKind::kEvent && events.getRegistry().findByName(name) != nullptr) {
        owner = TypeKind::kEvent;
    }
    if (owner != kind) {
        return fail(ErrorKind::kNameCollision,
                    std::format("{} '{}': the name is already used by {}", toLabel(kind), name, toNoun(owner)));
    }
    return {};
}
}  // namespace aylo::ecs::detail
