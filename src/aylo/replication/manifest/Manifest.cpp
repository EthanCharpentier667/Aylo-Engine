/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Manifest
*/

#include "aylo/replication/manifest/Manifest.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/world/World.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/replication/MessageType.hpp"
#include "aylo/replication/ReplicationSettings.hpp"
#include "aylo/replication/exceptions/ReplicationExceptions.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/manifest/ManifestMessages.hpp"
#include "aylo/replication/serialization/FieldSerializer.hpp"
#include "core/hash/NameHash.hpp"

namespace aylo::replication::manifest {
namespace {
/// @brief The most components a manifest lists: a short id is a `u16`.
constexpr std::size_t kMaxComponents = std::numeric_limits<std::uint16_t>::max();

/// @brief The most actions a manifest lists: an index is a `u8`.
constexpr std::size_t kMaxActions = std::numeric_limits<std::uint8_t>::max() + 1U;
}  // namespace

Manifest Manifest::build(const ecs::World& world, const input::InputLayout& inputs) {
    const auto& registry = world.getComponents().getRegistry();
    const std::span<const ecs::TypeId> replicated = registry.getReplicatedIds();
    if (replicated.size() > kMaxComponents) {
        throw exceptions::ManifestException{
            std::format("{} replicated components, the manifest holds at most {}", replicated.size(), kMaxComponents)};
    }
    std::vector<const ecs::ComponentInfo*> infos;
    infos.reserve(replicated.size());
    for (const ecs::TypeId id : replicated) {
        const ecs::ComponentInfo* info = registry.findById(id);
        if (info == nullptr) {
            continue;
        }
        if (!serialization::isSerializable(info->type)) {
            throw exceptions::ManifestException{
                std::format("The replicated component '{}' cannot be serialized: it needs at least one field and "
                            "plain data (no destruct, move or copy hook) or a copyable type with a list field",
                            info->type.name)};
        }
        if (info->type.name.size() > kMaxNameLength) {
            throw exceptions::ManifestException{std::format(
                "The replicated component '{}' has a name longer than {} bytes", info->type.name, kMaxNameLength)};
        }
        infos.push_back(info);
    }
    std::ranges::sort(infos, std::ranges::less{},
                      [](const ecs::ComponentInfo* info) -> const std::string& { return info->type.name; });
    if (inputs.actions().size() > kMaxActions) {
        throw exceptions::ManifestException{
            std::format("{} input actions, the manifest holds at most {}", inputs.actions().size(), kMaxActions)};
    }

    Manifest manifest;
    manifest._entries.reserve(infos.size());
    manifest._components.reserve(infos.size());
    for (const ecs::ComponentInfo* info : infos) {
        const auto shortId = static_cast<std::uint16_t>(manifest._entries.size());
        manifest._entries.push_back(
            {.name = info->type.name, .wireHash = serialization::computeManifestHash(info->type)});
        manifest._components.push_back(info->type.id);
        manifest._shorts.emplace(info->type.id, shortId);
    }
    manifest._actions.assign(inputs.actions().begin(), inputs.actions().end());
    manifest._hash = computeHash(manifest._entries, manifest._actions);
    return manifest;
}

std::vector<network::Bytes> Manifest::encode(const ReplicationSettings& settings) const {
    std::vector<network::Bytes> messages;
    messages.reserve(_entries.size() + _actions.size() + 1);
    std::uint16_t shortId = 0;
    for (const ComponentEntry& entry : _entries) {
        messages.push_back(ManifestCodec::encode(
            ManifestComponent{.shortId = shortId, .name = entry.name, .wireHash = entry.wireHash}));
        ++shortId;
    }
    std::uint8_t index = 0;
    for (const input::InputAction& action : _actions) {
        messages.push_back(
            ManifestCodec::encode(ManifestInputAction{.index = index, .name = action.name, .kind = action.kind}));
        ++index;
    }
    messages.push_back(ManifestCodec::encode(ManifestEnd{
        .entryCount = static_cast<std::uint16_t>(_entries.size() + _actions.size()),
        .hash = _hash,
        .tickRate = settings.tickRate,
        .snapshotRate = settings.snapshotRate,
        .reconnectionGraceSeconds = settings.reconnectionGraceSeconds,
    }));
    return messages;
}

std::uint64_t Manifest::hash() const noexcept { return _hash; }

std::span<const ComponentEntry> Manifest::componentEntries() const noexcept { return _entries; }

std::span<const input::InputAction> Manifest::actions() const noexcept { return _actions; }

std::span<const ecs::ComponentId> Manifest::components() const noexcept { return _components; }

std::optional<ecs::ComponentId> Manifest::toLocal(const std::uint16_t shortId) const noexcept {
    if (shortId >= _components.size()) {
        return std::nullopt;
    }
    return _components.at(shortId);
}

std::optional<std::uint16_t> Manifest::toShort(const ecs::ComponentId id) const noexcept {
    const auto found = _shorts.find(id);
    if (found == _shorts.end()) {
        return std::nullopt;
    }
    return found->second;
}

std::uint64_t Manifest::computeHash(const std::span<const ComponentEntry> components,
                                    const std::span<const input::InputAction> actions) noexcept {
    std::uint64_t hash = core::hash::kNameHashOffsetBasis;
    std::uint64_t shortId = 0;
    for (const ComponentEntry& entry : components) {
        hash = core::hash::hashInteger(hash, static_cast<std::uint64_t>(MessageType::kManifestComponent));
        hash = core::hash::hashInteger(hash, shortId);
        hash = core::hash::hashInteger(hash, entry.name.size());
        hash = core::hash::hashString(hash, entry.name);
        hash = core::hash::hashInteger(hash, entry.wireHash);
        ++shortId;
    }
    std::uint64_t index = 0;
    for (const input::InputAction& action : actions) {
        hash = core::hash::hashInteger(hash, static_cast<std::uint64_t>(MessageType::kManifestInputAction));
        hash = core::hash::hashInteger(hash, index);
        hash = core::hash::hashInteger(hash, action.name.size());
        hash = core::hash::hashString(hash, action.name);
        hash = core::hash::hashInteger(hash, static_cast<std::uint64_t>(action.kind));
        ++index;
    }
    return hash;
}
}  // namespace aylo::replication::manifest
