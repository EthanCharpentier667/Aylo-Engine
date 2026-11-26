/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SystemRun
*/

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "RuntimeHandle.hpp"
#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/DynamicCalls.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/system/DynamicComponents.hpp"
#include "aylo/ecs/system/DynamicEvents.hpp"
#include "aylo/ecs/system/DynamicQueries.hpp"
#include "aylo/ecs/system/DynamicResources.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/world/World.hpp"
#include "aylo/luau/FunctionRef.hpp"
#include "aylo/scripting/Export.hpp"
#include "aylo/scripting/view/ViewScope.hpp"

namespace aylo::scripting::system {
/// @brief A data term of a script system's query: what a script receives for it, after the entity.
struct DataTerm {
    const ecs::ComponentInfo* info{nullptr};  ///< The component; outlives the system (the registry never moves it).
    bool readOnly{false};                     ///< Whether the view refuses writes (a read term).
};

/// @brief A resource a script system uses: what `ctx.resources` gives for its name.
struct ResourceTerm {
    const ecs::TypeInfo* type{nullptr};  ///< The resource; outlives the system (the registry never moves it).
    bool write{false};                   ///< Whether the system may write it.
    bool discovered{false};  ///< Found while the system ran, not declared: a write turns its read into a write.
};

/// @brief A component a script system reads or writes on any entity: what `ctx:get` accepts for its name.
struct ComponentTerm {
    const ecs::ComponentInfo* info{nullptr};  ///< The component; outlives the system (the registry never moves it).
    bool write{false};                        ///< Whether the system may write it.
    bool discovered{false};  ///< Found while the system ran, not declared: a write turns its read into a write.
};

/// @brief An event type a script system uses: what `ctx.events` accepts for its name.
struct EventTerm {
    const ecs::TypeInfo* type{nullptr};  ///< The event; outlives the system (the registry never moves it).
    bool read{false};                    ///< Whether `ctx.events.read` accepts it.
    bool send{false};                    ///< Whether `ctx.events.send` accepts it.
    bool discovered{false};  ///< Found while the system ran, not declared: reading or sending it adds that use.
};

/// @brief What the body of a script system keeps between runs.
struct ScriptSystem {
    std::string name;                             ///< Name of the system, for the messages.
    luau::FunctionRef run;                        ///< The `run` function of the declaration.
    std::vector<DataTerm> terms;                  ///< The data terms of its query, in the order the query yields them.
    std::vector<ResourceTerm> resources;          ///< Its resources: those of its ResourceList, then those it found.
    std::vector<ComponentTerm> entityComponents;  ///< What `ctx:get` reaches: its ComponentList, then those it
                                                  ///< found.
    std::vector<EventTerm> events;                ///< Its event types: those of its EventList, then those it found.
    std::vector<const ecs::CallInfo*> calls;      ///< Its engine calls: its CallList, then those it found; owned by the
                                                  ///< world.
    bool clientSide{false};           ///< Runs on clients only: its commands cannot add replicated components.
    std::string_view side{"server"};  ///< `ctx.side`: "server" (the default), "client" or "both".
    const ecs::ComponentRegistry* components{nullptr};  ///< The world's components, for its commands.
    const ecs::World* world{nullptr};        ///< The world, to find the calls, resources and events it did not declare.
    std::shared_ptr<RuntimeHandle> runtime;  ///< The runtime of the host, null inside once it is destroyed.
    std::shared_ptr<view::ViewScope> scope;  ///< Invalidates the views of a run when it returns.
};

/// @brief Runs a script system once: calls its `run` with a `ctx`.
///
/// @details `ctx.query` is an iterator: `for entity, shield in ctx.query do` yields the entity and, for each data term
/// in the order of the query, a view of its component (read-only for a read term, `nil` for an optional component the
/// entity lacks). `ctx.time` is a table of the values of Time (`delta`, `fixedDelta`, `elapsed`, `tick`, `alpha`),
/// `ctx.delta` and `ctx.fixedDelta` stay as shorthands of its first two, and `ctx.commands` records structural changes
/// (see registerCommandsType()). `ctx.resources["aylo.Input"]` is a view of a resource (`nil` while it has no value),
/// `ctx.events` has `send(name, fields)` and `read(name)` (see registerContextTypes()), and `ctx.call(name, fields)`
/// makes an engine call (see call::callEngine()). `ctx:get(entity, name)` is a view of a component of any entity, `nil`
/// when the entity is dead or lacks it, and `ctx:has(entity, name)` says whether it has it.
///
/// A name the system declared is limited to what it declared: a resource or a component declared read refuses writes,
/// an event declared sent cannot be read. A name it did not declare is discovered at its first use and added to the
/// system's access (ecs::DynamicCalls::add() and the like): a resource or a component of `ctx:get` is read, then
/// written once a field of its view is written; an event is read or sent as it is used. An unknown name is a Luau
/// error. The views die when `run` returns. A Luau error in `run` is written to `std::cerr` with the name of the system
/// and skips the rest of the run; nothing is thrown.
/// @param system The system to run.
/// @param queries The system's queries, of which the first is iterated.
/// @param resources The system's resources, in the order of `system.resources`.
/// @param components The components `ctx:get` reaches, in the order of `system.entityComponents`.
/// @param events The system's event types, in the order of `system.events`.
/// @param calls The system's engine calls, in the order of `system.calls`.
/// @param time The time of the frame.
/// @param commands The buffer `ctx.commands` records into, flushed by the scheduler between phases.
AYLO_SCRIPTING_API void runScriptSystem(ScriptSystem& system, const ecs::DynamicQueries& queries,
                                        ecs::DynamicResources& resources, const ecs::DynamicComponents& components,
                                        const ecs::DynamicEvents& events, const ecs::DynamicCalls& calls,
                                        const ecs::Time& time, ecs::Commands& commands);
}  // namespace aylo::scripting::system
