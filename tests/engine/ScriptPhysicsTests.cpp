/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptPhysicsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <format>
#include <memory>
#include <string>
#include <utility>

#include "ScriptProbe.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginEntry.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/engine/Engine.hpp"
#include "aylo/engine/plugin/PluginLoader.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
constexpr int kFrames = 12;

/// @brief A server App with the physics, without gravity, and the replication plugin, one fixed tick per frame, and a
/// script host on it.
struct ServerScripts {
    aylo::engine::plugin::PluginLoader loader;  ///< The plugin libraries, which bring the setup forms on every side.
    aylo::ecs::App app{aylo::ecs::RunMode::kServer};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ServerScripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{20}, aylo::ecs::FixedTimestep{50, 1}));
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(aylo::plugins::physics::PhysicsPlugin{aylo::plugins::physics::PhysicsSettings{
            .gravity = {0.0F, 0.0F},
            .maxBodies = 16,
            .maxBodyPairs = 16,
            .maxContactConstraints = 16,
            .tempAllocatorBytes = 1024 * 1024,
        }});
        app.addPlugin(aylo::replication::ReplicationPlugin{});
        loader.load(aylo::core::system::getExecutableDirectory() / aylo::engine::Engine::kPluginsDirectory);
        aylo::core::library::LibraryRegistrar registrar;
        registrar.provide(aylo::ecs::kPluginRegistryService, app.getPluginRegistry());
        loader.registerInto(registrar);
        app.getPluginRegistry().addScriptForms("test.Probe", aylo::tests::kProbeForms);
        auto created = aylo::scripting::ScriptHost::create(app, {.isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("physics-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @brief Runs a few frames, enough for the bodies to be created and to meet.
    void runFrames() {
        for (int frame = 0; frame < kFrames; ++frame) {
            app.runFrame();
        }
    }
};
}  // namespace

TEST(ScriptPhysics, AScriptSpawnsBodiesAndReadsTheirCollision) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            query = {},
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = { position = { x = 0, y = 0 } },
                    ["aylo.Collider"] = { size = { x = 20, y = 20 } },
                    ["aylo.RigidBody"] = { gravityScale = 0, fixedRotation = true },
                    ["aylo.Velocity"] = { linear = { x = 600, y = 0 } },
                })
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = { position = { x = 40, y = 0 } },
                    ["aylo.Collider"] = { size = { x = 20, y = 20 } },
                })
            end,
        })
        probe.hits = 0
        engine.ecs.system("game.Hits", {
            phase = "Update",
            query = {},
            events = { read = { "aylo.CollisionStarted" } },
            run = function(ctx)
                for collision in ctx.events.read("aylo.CollisionStarted") do
                    probe.hits += 1
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    EXPECT_EQ(server.run("assert(probe.hits == 1, tostring(probe.hits))"), "");
}

TEST(ScriptPhysics, AScriptMovesAShipAndCastsARayThroughTheCalls) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Ship", { fields = {} })
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            query = {},
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = { position = { x = 100, y = 0 } },
                    ["aylo.Collider"] = { size = { x = 20, y = 200 } },
                })
                ctx.commands:spawn({
                    ["game.Ship"] = {},
                    ["aylo.Transform2D"] = { position = { x = 0, y = 0 } },
                    ["aylo.Collider"] = { shape = 1, radius = 10 },
                })
            end,
        })
        engine.ecs.system("game.Move", {
            phase = "FixedUpdate",
            query = { write = { "aylo.Transform2D" }, with = { "game.Ship" } },
            calls = { "aylo.physics.moveAndSlide", "aylo.physics.raycast" },
            after = { "aylo.Physics.SyncBodies" },
            before = { "aylo.Physics.SyncToPhysics" },
            run = function(ctx)
                for entity, transform in ctx.query do
                    transform.position = ctx.call("aylo.physics.moveAndSlide", {
                        entity = entity, from = transform.position, delta = { x = 50, y = 0 },
                    })
                    local hit = ctx.call("aylo.physics.raycast", {
                        origin = transform.position, direction = { x = 1, y = 0 }, maxDistance = 500, ignore = entity,
                    })
                    probe.shipX = transform.position.x
                    probe.wallDistance = if hit.hit then hit.distance else -1
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    // Stopped a skin (0.5) away from the wall's face at x = 90, which the ray then hits.
    EXPECT_EQ(server.run("assert(math.abs(probe.shipX - 79.5) < 0.1, tostring(probe.shipX))"), "");
    EXPECT_EQ(server.run("assert(math.abs(probe.wallDistance - 10.5) < 0.1, tostring(probe.wallDistance))"), "");
}

TEST(ScriptPhysics, AServerScriptSpawnsAShipForEachClientAndReadsItsInput) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.input.action("shoot")
        engine.input.action("move", "vector2")
        engine.ecs.system("game.Join", {
            phase = "Update",
            query = {},
            events = { read = { "net.ClientConnected" } },
            run = function(ctx)
                for client in ctx.events.read("net.ClientConnected") do
                    ctx.commands:spawn({ ["net.Owner"] = { client = client.client }, ["net.PlayerInput"] = {} })
                end
            end,
        })
        engine.ecs.system("game.Steer", {
            phase = "PostUpdate",
            query = { read = { "net.Owner" }, with = { "net.PlayerInput" } },
            calls = { "aylo.replication.readInput" },
            run = function(ctx)
                for entity, owner in ctx.query do
                    probe.seenOwner = owner.client
                    probe.seenShoot = ctx.call("aylo.replication.readInput", { entity = entity, action = "shoot" }).held
                    probe.seenMove = ctx.call("aylo.replication.readInput", { entity = entity, action = "move" }).vector
                end
            end,
        })
    )"),
              "");
    const auto& layout = server.app.getWorld().getResources().require<aylo::replication::input::InputLayout>();
    EXPECT_EQ(layout.find("shoot").kind, aylo::replication::input::InputKind::kButton);  // declared by the form
    EXPECT_EQ(layout.find("move").kind, aylo::replication::input::InputKind::kVector2);

    server.app.getWorld().getEvents().send(aylo::replication::event::ClientConnected{.client = 7});
    server.app.runFrame();

    EXPECT_EQ(server.run("assert(probe.seenOwner == 7 and probe.seenShoot == false and probe.seenMove.x == 0, "
                         "tostring(probe.seenOwner))"),
              "");
}

TEST(ScriptPhysics, AScriptNamesTheShapeAndTheBodyType) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = { position = { x = 0, y = 0 } },
                    ["aylo.Collider"] = { shape = "circle", radius = 8 },
                    ["aylo.RigidBody"] = { type = "kinematic" },
                })
            end,
        })
        engine.ecs.system("game.Read", {
            phase = "PostUpdate",
            query = { read = { "aylo.Collider", "aylo.RigidBody" } },
            run = function(ctx)
                for _, collider, body in ctx.query do
                    probe.shape = collider.shape
                    probe.type = body.type
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    EXPECT_EQ(server.run(R"(assert(probe.shape == "circle" and probe.type == "kinematic", tostring(probe.shape)))"),
              "");
}

TEST(ScriptPhysics, AScriptDropsA3DBallOnAFloorAndCastsA3DRay) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Ball", { fields = {} })
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(0, -0.5, 0) },
                    ["aylo.Collider3D"] = { shape = "box", size = vector.create(10, 1, 10) },
                })
                ctx.commands:spawn({
                    ["game.Ball"] = {},
                    ["aylo.Transform3D"] = { position = vector.create(0, 0.6, 0) },
                    ["aylo.Collider3D"] = { shape = "sphere", radius = 0.5 },
                    ["aylo.RigidBody"] = {},
                })
            end,
        })
        engine.ecs.system("game.Look", {
            phase = "FixedUpdate",
            after = { "aylo.Physics.Events" },
            query = { read = { "aylo.Transform3D", "aylo.Collider3D" }, with = { "game.Ball" } },
            calls = { "aylo.physics.raycast3D" },
            run = function(ctx)
                for entity, transform, collider in ctx.query do
                    local hit = ctx.call("aylo.physics.raycast3D", {
                        origin = vector.create(0, 5, 0), direction = vector.create(0, -1, 0), maxDistance = 10,
                        ignore = entity,
                    })
                    probe.ballY = transform.position.y
                    probe.shape = collider.shape
                    probe.floorDistance = if hit.hit then hit.distance else -1
                    probe.normalY = hit.normal.y
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    // The ball rests on the floor (its top at y = 0), and the ray, skipping it, hits the floor 5 m down.
    EXPECT_EQ(server.run("assert(math.abs(probe.ballY - 0.5) < 0.05, tostring(probe.ballY))"), "");
    EXPECT_EQ(server.run(R"(assert(probe.shape == "sphere", tostring(probe.shape)))"), "");
    EXPECT_EQ(server.run("assert(math.abs(probe.floorDistance - 5) < 0.01, tostring(probe.floorDistance))"), "");
    EXPECT_EQ(server.run("assert(math.abs(probe.normalY - 1) < 0.01, tostring(probe.normalY))"), "");
}

TEST(ScriptPhysics, TheManualsHopperPushesA3DBallUp) {
    ServerScripts server;
    // The recipe of the manual's 3D bodies section, with a floor, a ball and a probe of its height.
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Hopper", { fields = {} })
        engine.ecs.system("game.Hop", {
            phase = "FixedUpdate",
            query = { read = { "aylo.Transform3D" }, with = { "game.Hopper" } },
            calls = { "aylo.physics.raycast3D", "aylo.physics.applyImpulse3D" },
            after = { "aylo.Physics.SyncBodies" },
            before = { "aylo.Physics.SyncToPhysics" },
            run = function(ctx)
                for ball, transform in ctx.query do
                    local ground = ctx.call("aylo.physics.raycast3D", {
                        origin = transform.position, direction = vector.create(0, -1, 0), maxDistance = 0.55,
                        ignore = ball,
                    })
                    if ground.hit then
                        ctx.call("aylo.physics.applyImpulse3D", { entity = ball, value = vector.create(0, 4, 0) })
                    end
                end
            end,
        })
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(0, -0.5, 0) },
                    ["aylo.Collider3D"] = { shape = "box", size = vector.create(10, 1, 10) },
                })
                ctx.commands:spawn({
                    ["game.Hopper"] = {},
                    ["aylo.Transform3D"] = { position = vector.create(0, 0.5, 0) },
                    ["aylo.Collider3D"] = { shape = "sphere", radius = 0.5 },
                    ["aylo.RigidBody"] = {},
                })
            end,
        })
        probe.highest = 0
        engine.ecs.system("game.Watch", {
            phase = "FixedUpdate",
            after = { "aylo.Physics.SyncFromPhysics" },
            query = { read = { "aylo.Transform3D" }, with = { "game.Hopper" } },
            run = function(ctx)
                for _, transform in ctx.query do
                    probe.highest = math.max(probe.highest, transform.position.y)
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    // An impulse of 4 on 1 kg is 4 m/s up: well above the resting height (0.5) within a few ticks.
    EXPECT_EQ(server.run("assert(probe.highest > 0.6, tostring(probe.highest))"), "");
}

// The character of the manual (physics.md, "A character in 3D"), as it is written there.
constexpr const char* kManualCharacter = R"(
-- Your input system writes wish: where the player wants to go, length 0 to 1, and jump.
engine.ecs.component("game.Player", { fields = { wish = "vec3", jump = "bool", fall = "f32", grounded = "bool" } })

local speed = 5 -- metres per second
local gravity = -20 -- metres per second squared
local jumpSpeed = 7 -- metres per second, up

engine.ecs.system("game.Walk", {
	phase = "FixedUpdate",
	query = { write = { "aylo.Transform3D", "game.Player" } },
	calls = { "aylo.physics.moveAndSlide3D" },
	after = { "aylo.Physics.SyncBodies" },
	before = { "aylo.Physics.SyncToPhysics" },
	run = function(ctx)
		local dt = ctx.fixedDelta
		for entity, transform, player in ctx.query do
			if player.grounded then
				player.fall = if player.jump then jumpSpeed else 0
			end
			player.fall += gravity * dt
			local moved = ctx.call("aylo.physics.moveAndSlide3D", {
				entity = entity,
				from = transform.position,
				delta = player.wish * (speed * dt) + vector.create(0, player.fall * dt, 0),
				stepHeight = 0.3,
			})
			transform.position = moved.position
			player.grounded = moved.grounded
		end
	end,
})
)";

TEST(ScriptPhysics, TheManualsCharacterWalksUpAStepAndStopsAtAWall) {
    ServerScripts server;
    ASSERT_EQ(server.run(kManualCharacter), "");
    ASSERT_EQ(server.run(R"(
        local spawned = false
        engine.ecs.system("game.Level", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                -- the floor (top at y = 0), a step 0.25 m high from x = 1, a wall whose face is at x = 3.5
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(0, -0.5, 0) },
                    ["aylo.Collider3D"] = { shape = "box", size = vector.create(20, 1, 20) },
                })
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(2.5, 0.125, 0) },
                    ["aylo.Collider3D"] = { shape = "box", size = vector.create(3, 0.25, 20) },
                })
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(4, 2, 0) },
                    ["aylo.Collider3D"] = { shape = "box", size = vector.create(1, 4, 20) },
                })
                ctx.commands:spawn({
                    ["game.Player"] = { wish = vector.create(1, 0, 0) },
                    ["aylo.Transform3D"] = { position = vector.create(0, 1, 0) },
                    ["aylo.Collider3D"] = { shape = "capsule", radius = 0.3, halfHeight = 0.6 },
                    ["aylo.RigidBody"] = { type = "kinematic" },
                })
            end,
        })
        engine.ecs.system("game.Look", {
            phase = "FixedUpdate",
            after = { "aylo.Physics.Events" },  -- after the step, so after game.Walk too
            query = { read = { "aylo.Transform3D", "game.Player" } },
            calls = { "aylo.physics.shapeCast3D", "aylo.physics.overlap3D" },
            run = function(ctx)
                for entity, transform, player in ctx.query do
                    probe.x = transform.position.x
                    probe.y = transform.position.y
                    probe.grounded = player.grounded
                    local hit = ctx.call("aylo.physics.shapeCast3D", {
                        shape = "sphere", radius = 0.5, from = vector.create(0, 2, 0),
                        direction = vector.create(1, 0, 0), maxDistance = 10, ignore = entity,
                    })
                    probe.wallDistance = if hit.hit then hit.distance else -1
                    probe.under = ctx.call("aylo.physics.overlap3D", {
                        shape = "box", size = vector.create(0.5, 0.5, 0.5), at = vector.create(2, 0, 0), ignore = entity,
                    }).count
                end
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 60; ++frame) {
        server.app.runFrame();
    }

    // Walked 6 m worth: up the step (its top at 0.25) and to the wall, a skin and its radius from the face.
    EXPECT_EQ(server.run("assert(math.abs(probe.x - 3.18) < 0.02, tostring(probe.x))"), "");
    EXPECT_EQ(server.run("assert(math.abs(probe.y - 1.17) < 0.02, tostring(probe.y))"), "");
    EXPECT_EQ(server.run("assert(probe.grounded, 'not grounded')"), "");
    // A sphere from x = 0 touches the wall's face (x = 3.5) after 3 m; the box at x = 2 covers the floor and the step.
    EXPECT_EQ(server.run("assert(math.abs(probe.wallDistance - 3) < 0.01, tostring(probe.wallDistance))"), "");
    EXPECT_EQ(server.run("assert(probe.under == 2, tostring(probe.under))"), "");
}

TEST(ScriptPhysics, TheFormsMoveAShipAndQueryThe2DWorld) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Ship", { fields = {} })
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = { position = { x = 100, y = 0 } },
                    ["aylo.Collider"] = { size = { x = 20, y = 200 } },
                })
                ctx.commands:spawn({
                    ["game.Ship"] = {},
                    ["aylo.Transform2D"] = { position = { x = 0, y = 0 } },
                    ["aylo.Collider"] = { shape = "circle", radius = 10 },
                })
            end,
        })
        engine.ecs.system("game.Move", {
            phase = "FixedUpdate",
            query = { read = { "aylo.Collider" }, write = { "aylo.Transform2D" }, with = { "game.Ship" } },
            calls = { "aylo.physics.moveAndSlide", "aylo.physics.raycast", "aylo.physics.shapeCast", "aylo.physics.overlap" },
            after = { "aylo.Physics.SyncBodies" },
            before = { "aylo.Physics.SyncToPhysics" },
            run = function(ctx)
                for entity, collider, transform in ctx.query do
                    transform.position = engine.physics.moveAndSlide(ctx, entity, transform.position, { x = 50, y = 0 })
                    local wall = engine.physics.raycast(ctx, transform.position, { x = 1, y = 0 }, 500, { ignore = entity })
                    local cast = engine.physics.shapeCast(ctx, collider, { x = 0, y = 0 }, { x = 1, y = 0 }, 500, { ignore = entity })
                    local missed = engine.physics.raycast(ctx, transform.position, { x = -1, y = 0 }, 500, { ignore = entity })
                    local count, first = engine.physics.overlap(ctx, { shape = "box", size = { x = 40, y = 40 } }, { x = 100, y = 0 })
                    probe.shipX = transform.position.x
                    probe.wallDistance = wall.distance
                    probe.castDistance = cast.distance
                    probe.missed = missed == nil
                    probe.overlapCount = count
                    probe.overlapFirst = first ~= nil
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    // As the calls: stopped a skin (0.5) from the wall's face at x = 90, which the ray hits 10.5 further.
    EXPECT_EQ(server.run("assert(math.abs(probe.shipX - 79.5) < 0.1, tostring(probe.shipX))"), "");
    EXPECT_EQ(server.run("assert(math.abs(probe.wallDistance - 10.5) < 0.1, tostring(probe.wallDistance))"), "");
    // The ship's own collider, swept from x = 0, touches the face after 80.
    EXPECT_EQ(server.run("assert(math.abs(probe.castDistance - 80) < 0.1, tostring(probe.castDistance))"), "");
    EXPECT_EQ(server.run("assert(probe.missed, 'a ray that hits nothing gives nil')"), "");
    // The 40 x 40 box at x = 100 covers the wall and the ship, stopped against it.
    EXPECT_EQ(server.run("assert(probe.overlapCount == 2 and probe.overlapFirst, tostring(probe.overlapCount))"), "");
}

TEST(ScriptPhysics, TheFormsMoveACharacterAndQueryThe3DWorld) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Walker", { fields = {} })
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(0, -0.5, 0) },
                    ["aylo.Collider3D"] = { shape = "box", size = vector.create(20, 1, 20) },
                })
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(4, 2, 0) },
                    ["aylo.Collider3D"] = { shape = "box", size = vector.create(1, 4, 20) },
                })
                ctx.commands:spawn({
                    ["game.Walker"] = {},
                    ["aylo.Transform3D"] = { position = vector.create(0, 1, 0) },
                    ["aylo.Collider3D"] = { shape = "capsule", radius = 0.3, halfHeight = 0.6 },
                    ["aylo.RigidBody"] = { type = "kinematic" },
                })
            end,
        })
        engine.ecs.system("game.Walk", {
            phase = "FixedUpdate",
            query = { read = { "aylo.Collider3D" }, write = { "aylo.Transform3D" }, with = { "game.Walker" } },
            calls = {
                "aylo.physics.moveAndSlide3D", "aylo.physics.raycast3D", "aylo.physics.shapeCast3D",
                "aylo.physics.overlap3D",
            },
            after = { "aylo.Physics.SyncBodies" },
            before = { "aylo.Physics.SyncToPhysics" },
            run = function(ctx)
                for entity, collider, transform in ctx.query do
                    local moved = engine.physics.moveAndSlide3D(ctx, entity, transform.position,
                        vector.create(0.2, -0.1, 0), { stepHeight = 0.3 })
                    transform.position = moved.position
                    local ground = engine.physics.raycast3D(ctx, transform.position, vector.create(0, -1, 0), 5, { ignore = entity })
                    local cast = engine.physics.shapeCast3D(ctx, collider, vector.create(0, 2, 0), vector.create(1, 0, 0), 10,
                        { ignore = entity, rotation = engine.math.quat.identity() })
                    local count = engine.physics.overlap3D(ctx, { shape = "sphere", radius = 0.5 }, vector.create(4, 2, 0))
                    probe.x = transform.position.x
                    probe.grounded = moved.grounded
                    probe.groundDistance = ground.distance
                    probe.castDistance = cast.distance
                    probe.overlapCount = count
                end
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 40; ++frame) {
        server.app.runFrame();
    }

    // Walked to the wall (its face at x = 3.5), a skin and the radius from it, standing on the floor.
    EXPECT_EQ(server.run("assert(math.abs(probe.x - 3.18) < 0.02, tostring(probe.x))"), "");
    EXPECT_EQ(server.run("assert(probe.grounded, 'not grounded')"), "");
    EXPECT_EQ(server.run("assert(math.abs(probe.groundDistance - 0.92) < 0.02, tostring(probe.groundDistance))"), "");
    // The capsule (radius 0.3), swept from x = 0, touches the face after 3.2 m.
    EXPECT_EQ(server.run("assert(math.abs(probe.castDistance - 3.2) < 0.02, tostring(probe.castDistance))"), "");
    EXPECT_EQ(server.run("assert(probe.overlapCount == 1, tostring(probe.overlapCount))"), "");
}

TEST(ScriptPhysics, TheFormsSayWhatIsWrongWithTheirArguments) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.system("game.Misuse", {
            phase = "Update",
            calls = { "aylo.physics.raycast", "aylo.physics.shapeCast3D", "aylo.physics.moveAndSlide3D" },
            run = function(ctx)
                local function message(f)
                    local done, err = pcall(f)
                    return if done then "no error" else tostring(err)
                end
                probe.noContext = message(function() engine.physics.raycast(nil, { x = 0, y = 0 }, { x = 1, y = 0 }, 10) end)
                probe.badVector = message(function() engine.physics.raycast(ctx, 3, { x = 1, y = 0 }, 10) end)
                probe.badOption = message(function() engine.physics.raycast(ctx, { x = 0, y = 0 }, { x = 1, y = 0 }, 10, { ignor = 1 }) end)
                probe.misspelled = message(function()
                    engine.physics.shapeCast3D(ctx, { shape = "sphere", raduis = 1 }, vector.zero, vector.one, 10)
                end)
                probe.badRotation = message(function()
                    engine.physics.shapeCast3D(ctx, { shape = "sphere" }, vector.zero, vector.one, 10, { rotation = 1 })
                end)
                probe.noCollider = message(function()
                    engine.physics.moveAndSlide3D(ctx, ctx.commands:spawn({}), vector.zero, vector.one)
                end)
                probe.unlisted = message(function() engine.physics.overlap(ctx, { shape = "circle" }, { x = 0, y = 0 }) end)
            end,
        })
    )"),
              "");

    server.app.runFrame();

    const auto says = [&server](const std::string& field, const std::string& text) {
        return server.run(std::format("assert(string.find(probe.{}, {}, 1, true), probe.{})", field,
                                      std::format("\"{}\"", text), field));
    };
    EXPECT_EQ(says("noContext", "engine.physics.raycast: expected the system's ctx first"), "");
    EXPECT_EQ(says("badVector", "engine.physics.raycast: 'origin' must be a table { x, y }"), "");
    EXPECT_EQ(says("badOption", "engine.physics.raycast: the options"), "");
    EXPECT_EQ(says("misspelled", "engine.physics.shapeCast3D: the shape"), "");
    EXPECT_EQ(says("misspelled", "raduis"), "");
    EXPECT_EQ(says("badRotation", "'rotation' must be a quaternion"), "");
    EXPECT_EQ(says("noCollider", "engine.physics.moveAndSlide3D: "), "");
    EXPECT_EQ(says("noCollider", "has no Collider3D"), "");
    EXPECT_EQ(says("unlisted", "no error"), "");  // a call left out of `calls` is found when it is made
}

TEST(ScriptPhysics, TheFormsPushBodiesAndSetTheGravity) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Ball", { fields = {} })
        engine.ecs.component("game.Puck", { fields = {} })
        local spawned, pushed = false, false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["game.Ball"] = {},
                    ["aylo.Transform3D"] = { position = vector.create(0, 5, 0) },
                    ["aylo.Collider3D"] = { shape = "sphere", radius = 0.5 },
                    ["aylo.RigidBody"] = {},
                })
                ctx.commands:spawn({
                    ["game.Puck"] = {},
                    ["aylo.Transform2D"] = { position = { x = 0, y = 0 } },
                    ["aylo.Collider"] = { shape = "circle", radius = 10 },
                    ["aylo.RigidBody"] = {},
                })
            end,
        })
        engine.ecs.system("game.Push", {
            phase = "FixedUpdate",
            query = { read = { "game.Ball" } },
            calls = {
                "aylo.physics.setGravity3D", "aylo.physics.setGravity", "aylo.physics.applyImpulse3D",
                "aylo.physics.applyForce3D", "aylo.physics.applyTorque3D",
            },
            after = { "aylo.Physics.SyncBodies" },
            before = { "aylo.Physics.SyncToPhysics" },
            run = function(ctx)
                for ball in ctx.query do
                    if not pushed then
                        pushed = true
                        engine.physics.setGravity3D(ctx, vector.zero) -- it floats
                        engine.physics.setGravity(ctx, { x = 0, y = 0 })
                        engine.physics.applyImpulse3D(ctx, ball, vector.create(2, 0, 0))
                        engine.physics.applyForce3D(ctx, ball, { x = 0, y = 0, z = 0 })
                        engine.physics.applyTorque3D(ctx, ball, vector.create(0, 1, 0))
                    end
                end
            end,
        })
        engine.ecs.system("game.Puck", {
            phase = "FixedUpdate",
            query = { read = { "game.Puck" } },
            calls = { "aylo.physics.applyImpulse", "aylo.physics.applyForce", "aylo.physics.applyTorque" },
            after = { "aylo.Physics.SyncBodies", "game.Push" },
            before = { "aylo.Physics.SyncToPhysics" },
            run = function(ctx)
                for puck in ctx.query do
                    engine.physics.applyImpulse(ctx, puck, { x = 50, y = 0 })
                    engine.physics.applyForce(ctx, puck, { x = 0, y = 0 })
                    engine.physics.applyTorque(ctx, puck, 0)
                end
            end,
        })
        engine.ecs.system("game.Look", {
            phase = "FixedUpdate",
            after = { "aylo.Physics.Events" },
            query = { read = { "aylo.Transform3D" }, with = { "game.Ball" } },
            run = function(ctx)
                for _, transform in ctx.query do
                    probe.ballX = transform.position.x
                    probe.ballY = transform.position.y
                end
            end,
        })
        engine.ecs.system("game.LookPuck", {
            phase = "FixedUpdate",
            after = { "aylo.Physics.Events" },
            query = { read = { "aylo.Transform2D" }, with = { "game.Puck" } },
            run = function(ctx)
                for _, transform in ctx.query do
                    probe.puckX = transform.position.x
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    // No gravity: the ball keeps its height and goes the way it was pushed; the puck too.
    EXPECT_EQ(server.run("assert(math.abs(probe.ballY - 5) < 0.05, tostring(probe.ballY))"), "");
    EXPECT_EQ(server.run("assert(probe.ballX > 0.05, tostring(probe.ballX))"), "");
    EXPECT_EQ(server.run("assert(probe.puckX > 1, tostring(probe.puckX))"), "");
}

// The example of the manual's forms (physics.md, "Forms"), as it is written there.
constexpr const char* kManualForms = R"(
engine.ecs.system("game.Move", {
	phase = "FixedUpdate",
	query = { read = { "aylo.Collider" }, write = { "aylo.Transform2D" }, with = { "game.Ship" } },
	calls = { "aylo.physics.moveAndSlide", "aylo.physics.raycast", "aylo.physics.shapeCast" },
	after = { "aylo.Physics.SyncBodies" },
	before = { "aylo.Physics.SyncToPhysics" },
	run = function(ctx)
		for entity, collider, transform in ctx.query do
			transform.position = engine.physics.moveAndSlide(ctx, entity, transform.position, { x = 50, y = 0 })
			local wall = engine.physics.raycast(ctx, transform.position, { x = 1, y = 0 }, 500, { ignore = entity })
			if wall then
				print("a wall", wall.distance, "away")
			end
			-- the ship's own collider, swept forward
			local ahead = engine.physics.shapeCast(ctx, collider, transform.position, { x = 1, y = 0 }, 200, { ignore = entity })
		end
	end,
})
)";

TEST(ScriptPhysics, TheManualsFormsExampleMovesTheShip) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Ship", { fields = {} })
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = { position = { x = 100, y = 0 } },
                    ["aylo.Collider"] = { size = { x = 20, y = 200 } },
                })
                ctx.commands:spawn({
                    ["game.Ship"] = {},
                    ["aylo.Transform2D"] = { position = { x = 0, y = 0 } },
                    ["aylo.Collider"] = { shape = "circle", radius = 10 },
                })
            end,
        })
        engine.ecs.system("game.Look", {
            phase = "FixedUpdate",
            after = { "aylo.Physics.Events" },
            query = { read = { "aylo.Transform2D" }, with = { "game.Ship" } },
            run = function(ctx)
                for _, transform in ctx.query do
                    probe.shipX = transform.position.x
                end
            end,
        })
    )"),
              "");
    ASSERT_EQ(server.run(kManualForms), "");

    testing::internal::CaptureStderr();
    server.runFrames();
    const std::string errors = testing::internal::GetCapturedStderr();

    EXPECT_EQ(errors, "");
    EXPECT_EQ(server.run("assert(math.abs(probe.shipX - 79.5) < 0.1, tostring(probe.shipX))"), "");
}
