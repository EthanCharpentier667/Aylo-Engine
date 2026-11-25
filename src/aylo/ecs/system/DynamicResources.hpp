/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicResources
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
/// @brief One resource a system uses, by id, and whether it writes it.
struct ResourceUse {
    ResourceId id{kInvalidResourceId};  ///< The resource, registered before the system is added.
    bool write{false};                  ///< True when the system may write it, false when it only reads it.
};

/// @brief The configuration of a DynamicResources parameter: the resources the system uses, given when it is added.
/// @details They are numbered in this order (DynamicResources::get(0) is the first).
struct ResourceList {
    std::vector<ResourceUse> resources;  ///< The resources, in order.
};

/// @brief A system parameter giving the bytes of resources chosen when the system is added, by id, or found while it
/// runs: for resources the system does not know at compile time (those a script names).
///
/// @details Configured with a ResourceList (see SystemParam): the system's access set gets every resource of the list
/// as read or written, so the scheduler orders it as for `Res` and `ResMut`. A resource declared read refuses write
/// access (getMut() gives nothing), so the system writes nothing it did not declare. Main-thread resources are
/// refused: a dynamic system may run on any thread. add() gives the system a resource while it runs, or turns one it
/// reads into one it writes: its access grows, and the schedule checks the system again after the run.
/// @code
/// void run(aylo::ecs::DynamicResources resources) {
///     std::span<const std::byte> time = resources.get(0);
/// }
/// app.addSystem(aylo::ecs::Phase::kUpdate, "game.read", &run, aylo::ecs::ResourceList{{{timeId, false}}});
/// @endcode
class AYLO_ECS_API DynamicResources {
  public:
    /// @param resources The world's resources; they must outlive the view.
    /// @param list The resources of the system; it must outlive the view.
    /// @param access The system's access, grown by add(); null when the view is not built by a system's run.
    DynamicResources(Resources& resources, ResourceList& list, SystemAccess* access) noexcept;

    ~DynamicResources() = default;
    DynamicResources(const DynamicResources& other) noexcept = default;
    DynamicResources& operator=(const DynamicResources& other) noexcept = default;
    DynamicResources(DynamicResources&& other) noexcept = default;
    DynamicResources& operator=(DynamicResources&& other) noexcept = default;

    /// @return The number of resources: those of the ResourceList, then those add() found.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The use at `index`; `index` must be below getCount().
    [[nodiscard]] const ResourceUse& getUse(std::size_t index) const noexcept;

    /// @brief Gives the system the resource `id` while it runs, or turns its read into a write: adds it to the
    /// system's access (SystemContext::access).
    /// @return The index of the resource.
    /// @throws exceptions::ResourceRegistrationException When `id` is not a registered resource, is main-thread only,
    /// or the view is not built by a system's run (no access to grow).
    [[nodiscard]] std::size_t add(ResourceId id, bool write) const;

    /// @return Whether the resource at `index` holds a value.
    [[nodiscard]] bool has(std::size_t index) const noexcept;

    /// @return The bytes of the resource at `index`; empty if it has no value or is an empty type (use has()).
    [[nodiscard]] std::span<const std::byte> get(std::size_t index) const noexcept;

    /// @brief Write access to the resource at `index`, marked changed now.
    /// @return The bytes of the value; empty if the resource was declared read, has no value or is an empty type.
    [[nodiscard]] std::span<std::byte> getMut(std::size_t index) noexcept;

  private:
    Resources* _resources;  ///< The world's resources.
    ResourceList* _list;    ///< The system's resources.
    SystemAccess* _access;  ///< The system's access; null outside a system's run.
};

/// @brief `DynamicResources`: configured with a ResourceList; the system keeps it.
template <>
struct AYLO_ECS_API SystemParam<DynamicResources> {
    using Config = ResourceList;  ///< The resources to use, given when the system is added.
    using State = ResourceList;   ///< The same list, checked against the world.

    /// @return `config`, checked against the world's resources.
    /// @throws exceptions::ResourceRegistrationException When an id is not a registered resource, or the resource is
    /// main-thread only.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the resources for this run.
    [[nodiscard]] static DynamicResources get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds reading or writing every resource of the list.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace aylo::ecs
