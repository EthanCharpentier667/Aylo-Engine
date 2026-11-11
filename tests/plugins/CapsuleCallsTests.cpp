/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleCallsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/call/DynamicCalls.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ScriptCallable.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleCalls.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace aylo::test::capsule_calls {
struct Health {
    std::int32_t value;
};
AYLO_ECS_COMPONENT(Health, "test.capsulecalls.Health", value);
}  // namespace aylo::test::capsule_calls

namespace capsule = aylo::plugins::capsule;
namespace tc = aylo::test::capsule_calls;

namespace {
/// @return The bytes of a value, as a script's arguments.
template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}

/// @return The value held in result bytes.
template <typename T>
T valueOf(std::span<const std::byte> bytes) {
    T value{};
    std::memcpy(&value, bytes.data(), sizeof(T));
    return value;
}

/// @brief An App with the capsule plugin, "actors/turret.actor" (Health 5, a "Muzzle" child) and "scenes/level.scene"
/// (a turret child).
class CapsuleCallsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        app.addPlugin(capsule::CapsulePlugin{});
        defineTurret(tc::Health{.value = 5}, true);
        capsule::Capsule level{"scenes/level.scene"};
        level.editNode(level.addNode("Turret", 0)).instance = capsule::CapsuleRef{
            .id = capsule::CapsuleId::fromPath("actors/turret.actor"), .path = "actors/turret.actor"};
        capsules().define(std::move(level));
    }

    /// @return The world's capsules.
    capsule::Capsules& capsules() { return *app.getWorld().getResources().getMut<capsule::Capsules>(); }

    /// @brief Defines "actors/turret.actor" with `health`, and a "Muzzle" child if `muzzle`.
    void defineTurret(tc::Health health, bool muzzle) {
        capsule::Capsule turret{"actors/turret.actor"};
        turret.editNode(0).values.set(app.getWorld().getComponents(), health);
        if (muzzle) {
            static_cast<void>(turret.addNode("Muzzle", 0));
        }
        capsules().define(std::move(turret));
    }

    /// @return The id of a registered call.
    aylo::ecs::CallId callId(std::string_view name) { return app.getWorld().getCalls().findByName(name)->id; }

    /// @brief Calls `name` at once (as a script while it loads), with `arguments`, into a result of type `R`.
    template <typename R, typename A>
    R invoke(std::string_view name, const A& arguments) {
        std::array<std::byte, sizeof(R)> result{};
        app.getWorld().getCalls().invoke(app.getWorld(), callId(name), bytesOf(arguments), result);
        return valueOf<R>(result);
    }

    aylo::ecs::App app;  ///< The App with the calls.
};
}  // namespace

TEST_F(CapsuleCallsTest, RegistersTheThreeCalls) {
    EXPECT_NE(app.getWorld().getCalls().findByName("aylo.capsule.spawn"), nullptr);
    EXPECT_NE(app.getWorld().getCalls().findByName("aylo.capsule.load"), nullptr);
    EXPECT_NE(app.getWorld().getCalls().findByName("aylo.capsule.reload"), nullptr);
}

TEST_F(CapsuleCallsTest, SpawnsACapsule) {
    const auto spawned = invoke<capsule::CapsuleRoot>(
        capsule::kSpawnCall,
        capsule::CapsuleArguments{.capsule = aylo::core::asset::AssetId::fromPath("actors/turret.actor")});
    aylo::ecs::World& world = app.getWorld();
    ASSERT_TRUE(world.isAlive(spawned.root));  // the call's commands are applied when it returns
    EXPECT_EQ(world.getComponents().get<tc::Health>(spawned.root)->value, 5);
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST_F(CapsuleCallsTest, LoadsACapsuleInPlaceOfThePreviousOne) {
    const capsule::CapsuleArguments level{.capsule = aylo::core::asset::AssetId::fromPath("scenes/level.scene")};
    const capsule::CapsuleArguments turret{.capsule = aylo::core::asset::AssetId::fromPath("actors/turret.actor")};
    const auto first = invoke<capsule::CapsuleRoot>(capsule::kLoadCall, level);
    EXPECT_EQ(app.getWorld().getEntityCount(), 3U);
    const auto second = invoke<capsule::CapsuleRoot>(capsule::kLoadCall, turret);
    EXPECT_FALSE(app.getWorld().isAlive(first.root));
    EXPECT_EQ(app.getWorld().getResources().get<capsule::LoadedCapsule>()->root, second.root);
    EXPECT_EQ(app.getWorld().getEntityCount(), 2U);
}

TEST_F(CapsuleCallsTest, ReloadsARoot) {
    const auto spawned = invoke<capsule::CapsuleRoot>(
        capsule::kSpawnCall,
        capsule::CapsuleArguments{.capsule = aylo::core::asset::AssetId::fromPath("actors/turret.actor")});
    defineTurret(tc::Health{.value = 9}, false);
    EXPECT_TRUE(
        invoke<capsule::ReloadResult>(capsule::kReloadCall, capsule::ReloadArguments{.root = spawned.root}).reloaded);
    aylo::ecs::World& world = app.getWorld();
    EXPECT_EQ(world.getComponents().get<tc::Health>(spawned.root)->value, 9);
    EXPECT_EQ(world.getComponents().get<capsule::CapsuleInstance>(spawned.root)->version, 1U);
    EXPECT_EQ(world.getComponents().get<aylo::ecs::Children>(spawned.root), nullptr);  // no muzzle any more
    const aylo::ecs::Entity plain = world.spawn();
    EXPECT_FALSE(invoke<capsule::ReloadResult>(capsule::kReloadCall, capsule::ReloadArguments{.root = plain}).reloaded);
}

TEST_F(CapsuleCallsTest, RefusesAnUnknownCapsule) {
    EXPECT_THROW(static_cast<void>(invoke<capsule::CapsuleRoot>(
                     capsule::kSpawnCall,
                     capsule::CapsuleArguments{.capsule = aylo::core::asset::AssetId::fromPath("nowhere")})),
                 capsule::exceptions::CapsuleException);
    EXPECT_EQ(app.getWorld().getEntityCount(), 0U);
}

TEST_F(CapsuleCallsTest, SpawnsIntoTheCommandsOfTheSystemThatCalls) {
    aylo::ecs::Entity reserved;
    aylo::ecs::ScriptCallable callable{[&](const aylo::ecs::DynamicCalls calls) {
        const capsule::CapsuleArguments arguments{.capsule =
                                                      aylo::core::asset::AssetId::fromPath("actors/turret.actor")};
        std::array<std::byte, sizeof(capsule::CapsuleRoot)> result{};
        calls.invoke(0, bytesOf(arguments), result);
        reserved = valueOf<capsule::CapsuleRoot>(result).root;
    }};
    app.addSystem(aylo::ecs::Phase::kStartup, "test.spawnTurret", std::move(callable),
                  aylo::ecs::CallList{{callId(capsule::kSpawnCall)}});
    app.runFrame();
    ASSERT_TRUE(app.getWorld().isAlive(reserved));  // applied with the system's commands, after its phase
    EXPECT_EQ(app.getWorld().getComponents().get<tc::Health>(reserved)->value, 5);
}

TEST_F(CapsuleCallsTest, GivesTheSystemTheAccessOfTheCalls) {
    aylo::ecs::ScriptCallable callable{[](const aylo::ecs::DynamicCalls /*calls*/) {}};
    const aylo::ecs::System system{app.getWorld(), "test.load", std::move(callable),
                                   aylo::ecs::CallList{{callId(capsule::kLoadCall)}}};
    const auto capsules = app.getWorld().getResources().find("aylo.Capsules");
    const auto loaded = app.getWorld().getResources().find("aylo.LoadedCapsule");
    ASSERT_TRUE(capsules && loaded);
    bool readsCapsules = false;
    bool writesLoaded = false;
    for (const auto& resource : system.getAccess().getResources()) {
        readsCapsules = readsCapsules || (resource.id == *capsules && !resource.write);
        writesLoaded = writesLoaded || (resource.id == *loaded && resource.write);
    }
    EXPECT_TRUE(readsCapsules);
    EXPECT_TRUE(writesLoaded);
}

TEST(CapsuleCalls, ThePluginBringsItsSetupForms) {
    aylo::ecs::App app;
    app.addPlugin(capsule::CapsulePlugin{});
    const auto& forms = app.getPluginRegistry().getScriptForms();
    const auto found = forms.find("addCapsuleForms");
    ASSERT_NE(found, forms.end());
    EXPECT_TRUE(found->second.contains("engine.capsule.spawn"));
    EXPECT_TRUE(found->second.contains("engine.capsule.load"));
    EXPECT_TRUE(found->second.contains("engine.capsule.reload"));
    EXPECT_FALSE(found->second.ends_with('\0'));
}
