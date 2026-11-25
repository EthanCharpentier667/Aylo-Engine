/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicComponents
*/

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
/// @brief One component a system reads or writes on any entity, by id.
struct ComponentUse {
    ComponentId id{kInvalidComponentId};  ///< The component, registered before the system is added.
    bool write{false};                    ///< True when the system may write it, false when it only reads it.
};

/// @brief The configuration of a DynamicComponents parameter: the components the system uses on any entity, given
/// when it is added.
/// @details They are numbered in this order (DynamicComponents::get(entity, 0) is the first).
struct ComponentList {
    std::vector<ComponentUse> components;  ///< The components, in order.
};

/// @brief A system parameter giving the components of any entity, not only those a query visits: for a system that
/// follows a target, reads its parent or the entity it hit. The components are chosen when the system is added, by
/// id, or found while it runs (those a script names).
///
/// @details Configured with a ComponentList (see SystemParam): the system's access set gets every component of the
/// list as read or written, so the scheduler orders it as for a query. A component declared read refuses write access
/// (getMut() gives nothing). add() gives the system a component while it runs, or turns one it reads into one it
/// writes: its access grows, and the schedule checks the system again after the run. A system running on clients
/// cannot write a replicated or server-only component, as for its queries.
/// @code
/// void follow(aylo::ecs::DynamicComponents components, const aylo::ecs::Query<const Target> targets) {
///     for (const auto& [entity, target] : targets) {
///         std::span<const std::byte> health = components.get(target->entity, 0);
///     }
/// }
/// app.addSystem(aylo::ecs::Phase::kUpdate, "game.follow", &follow, aylo::ecs::ComponentList{{{healthId, false}}});
/// @endcode
class AYLO_ECS_API DynamicComponents {
  public:
    /// @param list The components of the system; it must outlive the view.
    /// @param context The run: its world, the access add() grows (null outside a system's run) and its side.
    DynamicComponents(ComponentList& list, const SystemContext& context) noexcept;

    ~DynamicComponents() = default;
    DynamicComponents(const DynamicComponents& other) noexcept = default;
    DynamicComponents& operator=(const DynamicComponents& other) noexcept = default;
    DynamicComponents(DynamicComponents&& other) noexcept = default;
    DynamicComponents& operator=(DynamicComponents&& other) noexcept = default;

    /// @return The number of components: those of the ComponentList, then those add() found.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The use at `index`; `index` must be below getCount().
    [[nodiscard]] const ComponentUse& getUse(std::size_t index) const noexcept;

    /// @return The index of component `id`, or nothing when the system does not use it.
    [[nodiscard]] std::optional<std::size_t> find(ComponentId id) const noexcept;

    /// @brief Gives the system the component `id` while it runs, or turns its read into a write: adds it to the
    /// system's access (SystemContext::access).
    /// @return The index of the component.
    /// @throws exceptions::ComponentRegistrationException When `id` is not a registered component, the view is not
    /// built by a system's run (no access to grow), or a system running on clients asks to write a replicated or
    /// server-only component.
    [[nodiscard]] std::size_t add(ComponentId id, bool write) const;

    /// @return Whether `entity` is alive and has the component at `index`.
    [[nodiscard]] bool has(Entity entity, std::size_t index) const noexcept;

    /// @return The bytes of the entity's component at `index`; empty if the entity is not alive, does not have it, or
    /// it is a tag (use has()). Invalidated by the next structural change to the component.
    [[nodiscard]] std::span<const std::byte> get(Entity entity, std::size_t index) const noexcept;

    /// @brief Write access to the entity's component at `index`, marked changed now.
    /// @return The bytes of the value; empty if the component was declared read, the entity is not alive, does not
    /// have it, or it is a tag.
    [[nodiscard]] std::span<std::byte> getMut(Entity entity, std::size_t index) const noexcept;

  private:
    ComponentList* _list;    ///< The system's components.
    SystemContext _context;  ///< The run: world, access and side.
};

/// @brief `DynamicComponents`: configured with a ComponentList; the system keeps it.
template <>
struct AYLO_ECS_API SystemParam<DynamicComponents> {
    using Config = ComponentList;  ///< The components to use, given when the system is added.
    using State = ComponentList;   ///< The same list, checked against the world.

    /// @return `config`, checked against the world's components.
    /// @throws exceptions::ComponentRegistrationException When an id is not a registered component.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the components for this run.
    [[nodiscard]] static DynamicComponents get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds reading or writing every component of the list.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace aylo::ecs
