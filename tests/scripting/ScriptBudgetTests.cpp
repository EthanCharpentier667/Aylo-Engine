/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptBudgetTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/luau/ErrorKind.hpp"
#include "aylo/scripting/ScriptHost.hpp"

namespace {
using std::chrono_literals::operator""ms;

constexpr std::size_t kMemoryLimit = std::size_t{32} * 1024 * 1024;

/// @brief An App with a host whose calls into scripts have @p callBudget and 32 MB, and a counter a healthy system
/// raises.
struct Fixture {
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    explicit Fixture(const std::chrono::milliseconds callBudget = 50ms) {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
        auto created =
            aylo::scripting::ScriptHost::create(app, {.memoryLimit = kMemoryLimit, .callBudget = callBudget});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_TRUE(host->run("components", R"(
            engine.ecs.component("game.Counter", { fields = { value = "f32" } })
            engine.ecs.system("game.Healthy", {
                phase = "Update",
                query = { write = { "game.Counter" } },
                run = function(ctx)
                    for _, counter in ctx.query do
                        counter.value += 1
                    end
                end,
            })
        )"));
        const auto* layout = host->findComponent("game.Counter");
        counter = app.getWorld().spawn();
        EXPECT_TRUE(app.getWorld().getComponents().add(counter, layout->id, layout->defaults));
    }

    /// @return How many frames game.Healthy ran.
    [[nodiscard]] float healthyRuns() const {
        const auto* layout = host->findComponent("game.Counter");
        const auto bytes = app.getWorld().getComponents().get(counter, layout->id);
        float value = 0.F;
        std::memcpy(&value, bytes.data(), sizeof(float));
        return value;
    }

    /// @return What two frames printed on the error output.
    [[nodiscard]] std::string twoFrames() {
        testing::internal::CaptureStderr();
        app.runFrame();
        app.runFrame();
        return testing::internal::GetCapturedStderr();
    }

    aylo::ecs::Entity counter{};
};
}  // namespace

TEST(ScriptBudget, AStuckSystemIsStoppedAndTheGameGoesOn) {
    Fixture fixture;
    ASSERT_TRUE(fixture.host->run("stuck", R"(
        engine.ecs.system("game.Stuck", {
            phase = "Update",
            before = { "game.Healthy" },
            run = function(ctx)
                while true do end
            end,
        })
    )"));

    const std::string log = fixture.twoFrames();

    EXPECT_NE(log.find("game.Stuck"), std::string::npos) << log;
    EXPECT_NE(log.find("ran longer than its budget of 50 ms"), std::string::npos) << log;
    EXPECT_FLOAT_EQ(fixture.healthyRuns(), 2.F);
}

TEST(ScriptBudget, AScriptThatLoopsWhenItLoadsFailsWithATimeout) {
    Fixture fixture;

    const auto ran = fixture.host->run("loops", "while true do end");

    ASSERT_FALSE(ran);
    EXPECT_EQ(ran.error().kind(), aylo::luau::ErrorKind::kTimeout);
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.healthyRuns(), 1.F);
}

TEST(ScriptBudget, ASystemThatAllocatesTooMuchIsStoppedAndTheGameGoesOn) {
    // A long budget: on a busy machine, the bomb must reach the memory limit before the time limit.
    Fixture fixture{std::chrono::seconds{30}};
    ASSERT_TRUE(fixture.host->run("bomb", R"(
        engine.ecs.system("game.Bomb", {
            phase = "Update",
            before = { "game.Healthy" },
            run = function(ctx)
                local t = {}
                for i = 1, 100000000 do t[i] = string.rep("x", 100) .. i end
            end,
        })
    )"));

    const std::string log = fixture.twoFrames();

    EXPECT_NE(log.find("game.Bomb"), std::string::npos) << log;
    EXPECT_NE(log.find("not enough memory"), std::string::npos) << log;
    EXPECT_FLOAT_EQ(fixture.healthyRuns(), 2.F);
}
