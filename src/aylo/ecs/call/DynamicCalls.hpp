/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicCalls
*/

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
/// @brief The configuration of a DynamicCalls parameter: the engine calls the system uses, given when it is added.
/// @details They are numbered in this order (DynamicCalls::invoke(0, ...) calls the first).
struct CallList {
    std::vector<CallId> calls;  ///< The calls, registered before the system is added.
};

/// @brief What a DynamicCalls parameter keeps between runs: the calls and the system's body of each.
struct DynamicCallsState {
    std::vector<const CallInfo*> infos;                    ///< The calls, in order.
    std::vector<std::unique_ptr<detail::CallBody>> owned;  ///< One body per distinct call.
    std::vector<detail::CallBody*> bodies;                 ///< The body of each call, in order; a call listed twice
                                                           ///< shares one.
};

/// @brief A system parameter that calls engine calls chosen when the system is added, by id, or found while it runs:
/// for the calls a script names.
///
/// @details Configured with a CallList (see SystemParam): the system gets a body of its own for every distinct call of
/// the list (a call listed twice shares it), and its access set gets what each of them reads and writes, so the
/// scheduler orders it as if it had the calls' parameters itself; a call that uses a main-thread resource puts the
/// system on the main thread. A call made through it builds its parameters with the system's run (its ticks and its
/// command buffer). add() gives the system a call it did not list, while it runs: its access grows by the call's, and
/// the schedule checks the system again after the run.
/// @code
/// void run(aylo::ecs::DynamicCalls calls) {
///     calls.invoke(0, arguments, result);
/// }
/// app.addSystem(aylo::ecs::Phase::kUpdate, "game.load", &run, aylo::ecs::CallList{{loadSoundId}});
/// @endcode
class AYLO_ECS_API DynamicCalls {
  public:
    /// @param state The system's calls; it must outlive the view.
    /// @param context The run the calls are made from.
    DynamicCalls(DynamicCallsState& state, const SystemContext& context) noexcept;

    ~DynamicCalls() = default;
    DynamicCalls(const DynamicCalls& other) noexcept = default;
    DynamicCalls& operator=(const DynamicCalls& other) noexcept = default;
    DynamicCalls(DynamicCalls&& other) noexcept = default;
    DynamicCalls& operator=(DynamicCalls&& other) noexcept = default;

    /// @return The number of calls: those of the CallList, then those add() found.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The call at `index`; `index` must be below getCount().
    [[nodiscard]] const CallInfo& getInfo(std::size_t index) const noexcept;

    /// @return The index of the call `id` among the system's calls, if it has it.
    [[nodiscard]] std::optional<std::size_t> find(CallId id) const noexcept;

    /// @brief Gives the system the call `id` while it runs, if it does not have it: binds a body for it and adds what
    /// it reads and writes to the system's access (SystemContext::access).
    /// @return The index of the call, to give invoke().
    /// @throws exceptions::CallException When `id` is not a registered call, the parameters are not built by a
    /// system's run (no access to grow), or the system runs on clients and the call writes a replicated or server-only
    /// component.
    [[nodiscard]] std::size_t add(CallId id) const;

    /// @brief Calls the call at `index`; `index` must be below getCount().
    /// @param arguments The bytes of its arguments (CallInfo::arguments), empty when it takes none.
    /// @param result Where its result is written (CallInfo::result), empty when it returns none.
    /// @throws exceptions::CallException When `arguments` or `result` does not have the size of the call's
    /// description, or is not aligned for call data that owns memory (see Calls::invoke()).
    /// @throws exceptions::MissingResourceException When a Res or ResMut parameter's resource has no value.
    void invoke(std::size_t index, std::span<const std::byte> arguments, std::span<std::byte> result) const;

  private:
    DynamicCallsState* _state;  ///< The system's calls and bodies.
    SystemContext _context;     ///< The run the calls are made from.
};

/// @brief `DynamicCalls`: configured with a CallList; the system keeps a body of each call.
template <>
struct AYLO_ECS_API SystemParam<DynamicCalls> {
    using Config = CallList;          ///< The calls to use, given when the system is added.
    using State = DynamicCallsState;  ///< The calls and the system's body of each.

    /// @return The calls of `config`, each with a body of the system's own.
    /// @throws exceptions::CallException When an id is not a registered call.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the calls for this run.
    [[nodiscard]] static DynamicCalls get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds what every call of the list reads and writes.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace aylo::ecs
