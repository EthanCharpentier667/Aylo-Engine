/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** main
*/

// Systems: a function (or a lambda without captures) whose parameters say what it reads and writes. The ECS deduces
// its access from the signature, which is what lets the scheduler order systems and detect conflicts.

#include <exception>
#include <format>
#include <iostream>
#include <string_view>

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/system/EventWriter.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/world/World.hpp"

using namespace aylo::ecs;

namespace {
struct Position {
    float x;
};
AYLO_ECS_COMPONENT(Position, "example.Position", x);

struct Velocity {
    float x;
};
AYLO_ECS_COMPONENT(Velocity, "example.Velocity", x);

struct Time {
    float delta;
};
AYLO_ECS_RESOURCE(Time, "example.Time", delta);

struct Moved {
    int count;
};
AYLO_ECS_EVENT(Moved, "example.Moved", count);

/// @brief Writes Position, reads Velocity and Time, sends Moved.
void movement(Query<Position, const Velocity> query, Res<Time> time, EventWriter<Moved> moved) {
    int count = 0;
    for (auto [entity, position, velocity] : query) {
        position->x += velocity.x * time->delta;
        ++count;
    }
    moved.send(Moved{.count = count});
    std::cout << std::format("  movement ran: {} entities moved\n", count);
}

/// @return The side's name.
std::string_view sideName(Side side) {
    switch (side) {
        case Side::kServer:
            return "server";
        case Side::kClient:
            return "client";
        case Side::kBoth:
            return "both";
    }
    return "?";
}

/// @brief Prints a system's name, side and what it reads and writes, with the type names from the registries.
void describe(const World& world, const System& system) {
    const SystemAccess& access = system.getAccess();
    std::cout << std::format("'{}' (side {}):\n", system.getName(), sideName(system.getSide()));
    for (const TypeAccess& component : access.getComponents()) {
        std::cout << std::format("  {} component '{}'\n", component.write ? "writes" : "reads ",
                                 world.getComponents().getRegistry().findById(component.id)->type.name);
    }
    for (const TypeAccess& resource : access.getResources()) {
        std::cout << std::format("  {} resource  '{}'\n", resource.write ? "writes" : "reads ",
                                 world.getResources().getRegistry().findById(resource.id)->type.name);
    }
    for (const TypeAccess& event : access.getEvents()) {
        std::cout << std::format("  {} event     '{}'\n", event.write ? "sends " : "reads ",
                                 world.getEvents().getRegistry().findById(event.id)->type.name);
    }
}

/// @brief Runs the example, printing each step.
void run() {
    World world;
    world.getResources().insert(Time{.delta = 0.5F});
    world.spawn(Position{.x = 0.F}, Velocity{.x = 2.F});
    world.spawn(Position{.x = 10.F}, Velocity{.x = -2.F});

    std::cout << "== A system made from a function: its access is deduced ==\n";
    System move{world, "example.movement", &movement};
    describe(world, move);

    std::cout << "\n== A system made from a lambda without captures ==\n";
    System spawner{world, "example.spawner", [](Commands& commands) { static_cast<void>(commands.spawn()); }};
    spawner.setSide(Side::kBoth);
    describe(world, spawner);
    std::cout << "  (Commands& adds no access: its changes are applied at the flush)\n";

    std::cout << "\n== Running ==\n";
    // Before its first run, a system's last run is set far in the past, so its first run sees everything as new.
    move.run();
    std::cout << std::format("after the run: last run at tick {}; the world tick moved on to {}\n", move.getLastRun(),
                             world.getChangeTick());
    spawner.run();
    std::cout << std::format("entities before the flush: {}\n", world.getEntityCount());
    spawner.flushCommands();
    std::cout << std::format("entities after the flush: {}\n", world.getEntityCount());
}
}  // namespace

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
