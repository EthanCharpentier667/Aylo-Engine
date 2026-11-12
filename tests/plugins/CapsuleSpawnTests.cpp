/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleSpawnTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/entity/Name.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/type/String.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace aylo::test::capsule_spawn {
struct Health {
    std::int32_t value;
};
AYLO_ECS_COMPONENT(Health, "test.capsulespawn.Health", value);

struct Position {
    float x;
};
AYLO_ECS_COMPONENT(Position, "test.capsulespawn.Position", x);

struct Path {
    std::vector<std::int32_t> points;
};
AYLO_ECS_COMPONENT(Path, "test.capsulespawn.Path");

/// @brief Registered as server-only.
struct Brain {
    std::int32_t state;
};
AYLO_ECS_COMPONENT(Brain, "test.capsulespawn.Brain", state);

/// @brief Added by the game, not by a capsule.
struct Marker {};
AYLO_ECS_COMPONENT(Marker, "test.capsulespawn.Marker");
}  // namespace aylo::test::capsule_spawn

namespace capsule = aylo::plugins::capsule;
namespace ts = aylo::test::capsule_spawn;

namespace {
/// @return The message of the CapsuleException `call` throws, or an empty string if it throws none.
template <typename Call>
std::string errorOf(Call call) {
    try {
        call();
    } catch (const capsule::exceptions::CapsuleException& error) {
        return error.what();
    }
    return "";
}

/// @return The id of a capsule's path.
capsule::CapsuleId idOf(std::string_view path) { return capsule::CapsuleId::fromPath(path); }

/// @return A reference to the capsule at `path`, for an instance node.
capsule::CapsuleRef refTo(std::string_view path) {
    return capsule::CapsuleRef{.id = idOf(path), .path = std::string{path}};
}

/// @brief An App with the capsule plugin and three capsules:
/// - "actors/turret.actor": a root (Health 5, Path {1, 2}) and a "Muzzle" child (Position 1);
/// - "actors/boss.actor": a root (Health 50), "Left" and "Right" turrets (Position -40 and 40), "Right" with an
///   "Extra" child (Position 99), and a client-side "Shadow" (Position 7);
/// - "scenes/level.scene": a root with no component, the boss at Position 600, and a client-side "Camera".
/// Standalone, the boss spawns 7 entities: its root, two turrets with their muzzles, the extra and the shadow.
class CapsuleSpawnTest : public ::testing::Test {
  protected:
    void SetUp() override {
        app.addPlugin(capsule::CapsulePlugin{});
        capsule::Capsule turret{"actors/turret.actor"};
        set(turret, 0, ts::Health{.value = 5});
        set(turret, 0, ts::Path{.points = {1, 2}});
        set(turret, turret.addNode("Muzzle", 0), ts::Position{.x = 1.F});
        capsules().define(std::move(turret));
        capsule::Capsule boss{"actors/boss.actor"};
        set(boss, 0, ts::Health{.value = 50});
        set(boss, instance(boss, "Left", "actors/turret.actor"), ts::Position{.x = -40.F});
        const std::uint32_t right = instance(boss, "Right", "actors/turret.actor");
        set(boss, right, ts::Position{.x = 40.F});
        set(boss, boss.addNode("Extra", right), ts::Position{.x = 99.F});
        const std::uint32_t shadow = boss.addNode("Shadow", 0);
        boss.editNode(shadow).side = aylo::ecs::Side::kClient;
        set(boss, shadow, ts::Position{.x = 7.F});
        capsules().define(std::move(boss));
        capsule::Capsule level{"scenes/level.scene"};
        set(level, instance(level, "Boss", "actors/boss.actor"), ts::Position{.x = 600.F});
        level.editNode(level.addNode("Camera", 0)).side = aylo::ecs::Side::kClient;
        capsules().define(std::move(level));
    }

    /// @return The world.
    aylo::ecs::World& world() { return app.getWorld(); }

    /// @return The world's capsules.
    capsule::Capsules& capsules() { return *world().getResources().getMut<capsule::Capsules>(); }

    /// @brief Gives a node of `capsule` a value.
    template <typename T>
    void set(capsule::Capsule& capsule, std::uint32_t node, T value) {
        capsule.editNode(node).values.set(world().getComponents(), std::move(value));
    }

    /// @return A new child of the root of `capsule` that instances the capsule at `path`.
    static std::uint32_t instance(capsule::Capsule& capsule, std::string name, std::string_view path) {
        const std::uint32_t node = capsule.addNode(std::move(name), 0);
        capsule.editNode(node).instance = refTo(path);
        return node;
    }

    /// @return The root of the capsule at `path`, spawned through commands, flushed.
    aylo::ecs::Entity spawn(std::string_view path) {
        aylo::ecs::Commands commands{world()};
        const aylo::ecs::Entity root = capsule::spawnCapsule(commands, capsules(), idOf(path)).getEntity();
        commands.flush();
        return root;
    }

    /// @return The root of the capsule at `path`, loaded through commands, flushed.
    aylo::ecs::Entity load(std::string_view path) {
        aylo::ecs::Commands commands{world()};
        capsule::LoadedCapsule& loaded = *world().getResources().getMut<capsule::LoadedCapsule>();
        const aylo::ecs::Entity root = capsule::loadCapsule(commands, capsules(), loaded, idOf(path)).getEntity();
        commands.flush();
        return root;
    }

    /// @brief Reloads the capsule spawned at `root` through commands, flushed.
    void reload(aylo::ecs::Entity root) {
        aylo::ecs::Commands commands{world()};
        const std::vector<aylo::ecs::Entity> children = childrenOf(root);
        capsule::reloadCapsule(commands, capsules(), root, *get<capsule::CapsuleInstance>(root), children);
        commands.flush();
    }

    /// @return The entity's component, or nullptr.
    template <typename T>
    const T* get(aylo::ecs::Entity entity) {
        return world().getComponents().get<T>(entity);
    }

    /// @return The entity's children, none if it has no Children.
    std::vector<aylo::ecs::Entity> childrenOf(aylo::ecs::Entity entity) {
        const auto* children = get<aylo::ecs::Children>(entity);
        return children != nullptr ? children->entities : std::vector<aylo::ecs::Entity>{};
    }

    aylo::ecs::App app;  ///< The App holding the capsules.
};
}  // namespace

TEST_F(CapsuleSpawnTest, SpawnsACopyOfEachValueWithTheChildrenParented) {
    const aylo::ecs::Entity root = spawn("actors/turret.actor");
    EXPECT_EQ(get<ts::Health>(root)->value, 5);
    EXPECT_EQ(get<ts::Path>(root)->points, (std::vector<std::int32_t>{1, 2}));
    const std::vector<aylo::ecs::Entity> children = childrenOf(root);
    ASSERT_EQ(children.size(), 1U);
    EXPECT_FLOAT_EQ(get<ts::Position>(children[0])->x, 1.F);
    EXPECT_EQ(get<aylo::ecs::Parent>(children[0])->entity, root);
    const auto* instance = get<capsule::CapsuleInstance>(root);
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->capsule, idOf("actors/turret.actor"));
    EXPECT_EQ(instance->version, 0U);
    EXPECT_FALSE(instance->nested);
    EXPECT_EQ(get<capsule::CapsuleInstance>(children[0]), nullptr);  // only roots
    EXPECT_EQ(world().getEntityCount(), 2U);
}

TEST_F(CapsuleSpawnTest, NamesEachEntityAfterItsNode) {
    const aylo::ecs::Entity root = spawn("actors/turret.actor");
    ASSERT_NE(get<aylo::ecs::Name>(root), nullptr);
    EXPECT_EQ(get<aylo::ecs::Name>(root)->value.view(), "turret");  // the root: its file's name
    const std::vector<aylo::ecs::Entity> children = childrenOf(root);
    ASSERT_EQ(children.size(), 1U);
    EXPECT_EQ(get<aylo::ecs::Name>(children[0])->value.view(), "Muzzle");
}

TEST_F(CapsuleSpawnTest, NamesAnInstanceAfterTheNodeThatPlacesIt) {
    const aylo::ecs::Entity boss = spawn("actors/boss.actor");
    EXPECT_EQ(get<aylo::ecs::Name>(boss)->value.view(), "boss");
    const std::vector<aylo::ecs::Entity> children = childrenOf(boss);
    ASSERT_EQ(children.size(), 3U);
    EXPECT_EQ(get<aylo::ecs::Name>(children[0])->value.view(), "Left");  // not "turret"
    EXPECT_EQ(get<aylo::ecs::Name>(children[1])->value.view(), "Right");
    EXPECT_EQ(get<aylo::ecs::Name>(childrenOf(children[0])[0])->value.view(), "Muzzle");
}

TEST_F(CapsuleSpawnTest, KeepsTheNameANodeGives) {
    capsule::Capsule named{"actors/named.actor"};
    set(named, 0, aylo::ecs::Name{.value = aylo::ecs::String{"Captain"}});
    capsules().define(std::move(named));
    const aylo::ecs::Entity root = spawn("actors/named.actor");
    EXPECT_EQ(get<aylo::ecs::Name>(root)->value.view(), "Captain");
}

TEST_F(CapsuleSpawnTest, SpawnsNestedCapsulesWithTheNodesValuesOverTheirRoots) {
    const aylo::ecs::Entity boss = spawn("actors/boss.actor");
    EXPECT_EQ(get<ts::Health>(boss)->value, 50);
    const std::vector<aylo::ecs::Entity> children = childrenOf(boss);
    ASSERT_EQ(children.size(), 3U);
    const aylo::ecs::Entity left = children[0];
    EXPECT_EQ(get<ts::Health>(left)->value, 5);          // the turret's root
    EXPECT_FLOAT_EQ(get<ts::Position>(left)->x, -40.F);  // with the boss node's value over it
    EXPECT_TRUE(get<capsule::CapsuleInstance>(left)->nested);
    EXPECT_EQ(get<capsule::CapsuleInstance>(left)->capsule, idOf("actors/turret.actor"));
    EXPECT_EQ(childrenOf(left).size(), 1U);  // its muzzle
    const std::vector<aylo::ecs::Entity> right = childrenOf(children[1]);
    ASSERT_EQ(right.size(), 2U);
    EXPECT_FLOAT_EQ(get<ts::Position>(right[0])->x, 1.F);   // the turret's muzzle
    EXPECT_FLOAT_EQ(get<ts::Position>(right[1])->x, 99.F);  // and the boss's extra child, under the instanced root
    EXPECT_FLOAT_EQ(get<ts::Position>(children[2])->x, 7.F);
    EXPECT_EQ(world().getEntityCount(), 7U);
}

TEST_F(CapsuleSpawnTest, SpawnsALevelWhoseRootHasNoComponentOfItsOwn) {
    const aylo::ecs::Entity level = spawn("scenes/level.scene");
    EXPECT_EQ(get<ts::Position>(level), nullptr);
    const std::vector<aylo::ecs::Entity> children = childrenOf(level);
    ASSERT_EQ(children.size(), 2U);
    EXPECT_FLOAT_EQ(get<ts::Position>(children[0])->x, 600.F);
    EXPECT_EQ(childrenOf(children[0]).size(), 3U);
    EXPECT_EQ(world().getEntityCount(), 9U);
}

TEST_F(CapsuleSpawnTest, SpawnsTheNodesOfItsSideWithTheirSubtrees) {
    capsules().setRunMode(aylo::ecs::RunMode::kServer);
    const aylo::ecs::Entity onServer = spawn("actors/boss.actor");
    EXPECT_EQ(childrenOf(onServer).size(), 2U);  // no shadow
    capsules().setRunMode(aylo::ecs::RunMode::kClient);
    const aylo::ecs::Entity onClient = spawn("scenes/level.scene");
    const std::vector<aylo::ecs::Entity> children = childrenOf(onClient);
    ASSERT_EQ(children.size(), 1U);  // the camera; the boss is the server's, with everything under it
    EXPECT_EQ(get<ts::Position>(children[0]), nullptr);
    EXPECT_EQ(world().getEntityCount(), 8U);  // the server's boss (6) and the client's level (2)
}

TEST_F(CapsuleSpawnTest, LeavesOutTheComponentsOfTheOtherSide) {
    static_cast<void>(world().getComponents().registerType<ts::Brain>({.presence = aylo::ecs::Presence::kServerOnly}));
    capsule::Capsule drone{"actors/drone.actor"};
    set(drone, 0, ts::Brain{.state = 1});
    set(drone, 0, ts::Health{.value = 1});
    capsules().define(std::move(drone));
    capsules().setRunMode(aylo::ecs::RunMode::kClient);
    const aylo::ecs::Entity onClient = spawn("actors/drone.actor");
    EXPECT_FALSE(world().getComponents().has<ts::Brain>(onClient));
    EXPECT_EQ(get<ts::Health>(onClient)->value, 1);
    capsules().setRunMode(aylo::ecs::RunMode::kServer);
    EXPECT_TRUE(world().getComponents().has<ts::Brain>(spawn("actors/drone.actor")));
}

TEST_F(CapsuleSpawnTest, ReservesTheRootAtOnceAndAppliesAtTheFlush) {
    aylo::ecs::Commands commands{world()};
    const aylo::ecs::Entity boss =
        capsule::spawnCapsule(commands, capsules(), idOf("actors/boss.actor")).add(ts::Health{.value = 80}).getEntity();
    EXPECT_FALSE(world().isAlive(boss));  // reserved until the flush
    commands.flush();
    ASSERT_TRUE(world().isAlive(boss));
    EXPECT_EQ(get<ts::Health>(boss)->value, 80);  // the add replaced the capsule's value
    ASSERT_EQ(childrenOf(boss).size(), 3U);
    EXPECT_FLOAT_EQ(get<ts::Position>(childrenOf(boss)[0])->x, -40.F);
    EXPECT_EQ(world().getEntityCount(), 7U);
}

TEST_F(CapsuleSpawnTest, EachSpawnIsAnIndependentCopy) {
    const aylo::ecs::Entity first = spawn("actors/turret.actor");
    const aylo::ecs::Entity second = spawn("actors/turret.actor");
    world().getComponents().getMut<ts::Path>(first)->points.push_back(3);
    EXPECT_EQ(get<ts::Path>(second)->points, (std::vector<std::int32_t>{1, 2}));
}

TEST_F(CapsuleSpawnTest, RefusesWhatCannotBeSpawnedAndRecordsNothing) {
    capsule::Capsule broken{"actors/broken.actor"};
    static_cast<void>(instance(broken, "Gun", "actors/missing.actor"));
    capsules().define(std::move(broken));
    capsule::Capsule a{"loop/a.capsule"};
    static_cast<void>(instance(a, "B", "loop/b.capsule"));
    capsules().define(std::move(a));
    capsule::Capsule b{"loop/b.capsule"};
    static_cast<void>(instance(b, "A", "loop/a.capsule"));
    capsules().define(std::move(b));
    aylo::ecs::Commands commands{world()};
    EXPECT_EQ(
        errorOf([&] { static_cast<void>(capsule::spawnCapsule(commands, capsules(), idOf("actors/missing.actor"))); }),
        std::format("No capsule has the id {:#x}", idOf("actors/missing.actor").value));
    EXPECT_EQ(
        errorOf([&] { static_cast<void>(capsule::spawnCapsule(commands, capsules(), idOf("actors/broken.actor"))); }),
        "Capsule 'actors/broken.actor' spawns 'actors/missing.actor' at 'Gun', which no capsule has");
    EXPECT_EQ(errorOf([&] { static_cast<void>(capsule::spawnCapsule(commands, capsules(), idOf("loop/a.capsule"))); }),
              "Capsule 'loop/a.capsule' contains itself: 'loop/a.capsule' -> 'loop/b.capsule' -> 'loop/a.capsule'");
    EXPECT_TRUE(commands.isEmpty());
    commands.flush();
    EXPECT_EQ(world().getEntityCount(), 0U);
}

TEST_F(CapsuleSpawnTest, LoadsACapsuleInPlaceOfTheOneLoadedBefore) {
    const aylo::ecs::Entity player = world().spawn(ts::Marker{});
    const aylo::ecs::Entity first = load("scenes/level.scene");
    EXPECT_EQ(world().getResources().get<capsule::LoadedCapsule>()->root, first);
    const aylo::ecs::Entity second = load("actors/turret.actor");
    EXPECT_FALSE(world().isAlive(first));
    EXPECT_TRUE(world().isAlive(player));  // outside the old capsule's tree
    const auto* loaded = world().getResources().get<capsule::LoadedCapsule>();
    EXPECT_EQ(loaded->root, second);
    EXPECT_EQ(loaded->capsule, idOf("actors/turret.actor"));
    EXPECT_EQ(world().getEntityCount(), 3U);     // the player and the turret
    static_cast<void>(world().despawn(second));  // the game despawned it: the next load does not mind
    static_cast<void>(load("actors/turret.actor"));
    EXPECT_EQ(world().getEntityCount(), 3U);
}

TEST_F(CapsuleSpawnTest, ALoadThatFailsChangesNothing) {
    const aylo::ecs::Entity first = load("actors/turret.actor");
    aylo::ecs::Commands commands{world()};
    capsule::LoadedCapsule& loaded = *world().getResources().getMut<capsule::LoadedCapsule>();
    EXPECT_FALSE(errorOf([&] {
                     static_cast<void>(capsule::loadCapsule(commands, capsules(), loaded, idOf("nowhere")));
                 }).empty());
    EXPECT_EQ(loaded.root, first);  // unchanged by a load that failed
    EXPECT_TRUE(commands.isEmpty());
}

TEST_F(CapsuleSpawnTest, ReloadsTheNewVersionIntoTheSameRoot) {
    const aylo::ecs::Entity boss = spawn("actors/boss.actor");
    static_cast<void>(world().getComponents().add(boss, ts::Marker{}));
    const std::vector<aylo::ecs::Entity> old = childrenOf(boss);
    capsule::Capsule again{"actors/boss.actor"};
    set(again, 0, ts::Health{.value = 60});
    static_cast<void>(instance(again, "Left", "actors/turret.actor"));
    capsules().define(std::move(again));
    reload(boss);

    ASSERT_TRUE(world().isAlive(boss));
    EXPECT_EQ(get<ts::Health>(boss)->value, 60);
    EXPECT_TRUE(world().getComponents().has<ts::Marker>(boss));  // the game's component stays
    EXPECT_EQ(get<capsule::CapsuleInstance>(boss)->version, 1U);
    EXPECT_EQ(childrenOf(boss).size(), 1U);
    for (const aylo::ecs::Entity child : old) {
        EXPECT_FALSE(world().isAlive(child));
    }
    EXPECT_EQ(world().getEntityCount(), 3U);  // the root, the turret and its muzzle
}

TEST_F(CapsuleSpawnTest, RefusesToReloadWhatCannotBeSpawnedAndRecordsNothing) {
    const aylo::ecs::Entity boss = spawn("actors/boss.actor");
    capsule::Capsule broken{"actors/boss.actor"};
    static_cast<void>(instance(broken, "Gun", "actors/missing.actor"));
    capsules().define(std::move(broken));
    aylo::ecs::Commands commands{world()};
    const std::vector<aylo::ecs::Entity> children = childrenOf(boss);
    EXPECT_FALSE(errorOf([&] {
                     capsule::reloadCapsule(commands, capsules(), boss, *get<capsule::CapsuleInstance>(boss), children);
                 }).empty());
    EXPECT_TRUE(commands.isEmpty());
    EXPECT_EQ(childrenOf(boss).size(), 3U);  // the old children are kept
}
