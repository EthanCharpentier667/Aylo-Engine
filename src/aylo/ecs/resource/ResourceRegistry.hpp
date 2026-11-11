/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceRegistry
*/

#pragma once

#include <string_view>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/type/TypeRegistry.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"

namespace aylo::ecs {
/// @brief How a C++ resource is sent over the network and which thread may use it, chosen when it is registered.
struct ResourceOptions {
    Presence presence{Presence::kBoth};                   ///< On which side of the network it exists.
    ReplicationMode replication{ReplicationMode::kNone};  ///< How the server sends it.
    bool nonSend{false};                                  ///< Main-thread only (the platform, the renderer).
};

/// @brief Compares what only resources have when a resource name is registered again: the main-thread flag.
/// @return Nothing when it matches, or an ErrorKind::kOptionsMismatch error naming both values.
[[nodiscard]] AYLO_ECS_API Result<void> checkSameThread(const ResourceInfo& registered, const ResourceInfo& requested);

/// @brief What the registry needs to know about resources.
template <>
struct KindTraits<ResourceInfo> {
    using Options = ResourceOptions;                        ///< Presence, replication, main thread.
    static constexpr TypeKind kKind = TypeKind::kResource;  ///< Declared with AYLO_ECS_RESOURCE.
    static constexpr std::string_view kName = "resource";   ///< The kind in messages.
    static constexpr std::string_view kLabel = "Resource";  ///< The kind at the start of a message.

    /// @return The rules of every type; a resource may be move-only.
    [[nodiscard]] static Result<void> validate(const ResourceInfo& info) { return aylo::ecs::validate(info); }

    /// @return Nothing when the main-thread flag matches, or why it does not.
    [[nodiscard]] static Result<void> checkSameOptions(const ResourceInfo& registered, const ResourceInfo& requested) {
        return checkSameThread(registered, requested);
    }

    /// @return The description of the C++ resource `T` with these options.
    template <Resource T>
    [[nodiscard]] static ResourceInfo make(const ResourceOptions& options);
};

/// @brief The catalog of the resource types of one world: a TypeRegistry of ResourceInfo.
/// @code
/// aylo::ecs::ResourceRegistry registry;
/// const aylo::ecs::ResourceId backend = registry.registerType<Backend>({.nonSend = true});
/// const aylo::ecs::ResourceInfo* time = registry.findByName("rtype.Time");
/// @endcode
using ResourceRegistry = TypeRegistry<ResourceInfo>;
}  // namespace aylo::ecs

#include "ResourceRegistry.tpp"
