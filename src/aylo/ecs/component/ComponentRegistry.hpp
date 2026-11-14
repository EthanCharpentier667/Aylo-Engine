/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentRegistry
*/

#pragma once

#include <string_view>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/type/TypeRegistry.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"

namespace aylo::ecs {
/// @brief How a C++ component is stored and sent over the network, chosen when it is registered.
struct ComponentOptions {
    StorageKind storage{StorageKind::kSparseSet};         ///< Where the values live.
    Presence presence{Presence::kBoth};                   ///< On which side of the network it exists.
    ReplicationMode replication{ReplicationMode::kNone};  ///< How the server sends it.
};

/// @brief Compares what only components have when a component name is registered again: the storage.
/// @return Nothing when it matches, or an ErrorKind::kOptionsMismatch error naming both values.
[[nodiscard]] AYLO_ECS_API Result<void> checkSameStorage(const ComponentInfo& registered,
                                                         const ComponentInfo& requested);

/// @brief What the registry needs to know about components.
template <>
struct KindTraits<ComponentInfo> {
    using Options = ComponentOptions;                        ///< Storage, presence, replication.
    static constexpr TypeKind kKind = TypeKind::kComponent;  ///< Declared with AYLO_ECS_COMPONENT.
    static constexpr std::string_view kName = "component";   ///< The kind in messages.
    static constexpr std::string_view kLabel = "Component";  ///< The kind at the start of a message.

    /// @return The rules of every type, and a component must be copyable.
    [[nodiscard]] static Result<void> validate(const ComponentInfo& info) { return aylo::ecs::validate(info); }

    /// @return Nothing when the storage matches, or why it does not.
    [[nodiscard]] static Result<void> checkSameOptions(const ComponentInfo& registered,
                                                       const ComponentInfo& requested) {
        return checkSameStorage(registered, requested);
    }

    /// @return The description of the C++ component `T` with these options.
    template <Component T>
    [[nodiscard]] static ComponentInfo make(const ComponentOptions& options);
};

/// @brief The catalog of the component types of one world: a TypeRegistry of ComponentInfo.
/// @code
/// aylo::ecs::ComponentRegistry registry;
/// const aylo::ecs::ComponentId transform =
///     registry.registerType<Transform2D>({.replication = aylo::ecs::ReplicationMode::kEveryChange});
/// const aylo::ecs::ComponentInfo* info = registry.findByName("aylo.Transform2D");
/// @endcode
using ComponentRegistry = TypeRegistry<ComponentInfo>;
}  // namespace aylo::ecs

#include "ComponentRegistry.tpp"
