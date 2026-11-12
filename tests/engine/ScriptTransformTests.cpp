/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptTransformTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryState.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
using aylo::plugins::transform::GlobalTransform;
using aylo::plugins::transform::Transform;
using aylo::plugins::transform::TransformInterpolation;

/// @brief A single-player App whose frames last half a fixed tick (10 ticks per second, 50 ms frames), with the
/// TransformPlugin and a script host on it.
struct Scripts {
    aylo::ecs::App app{aylo::ecs::RunMode::kStandalone};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Scripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{50}, aylo::ecs::FixedTimestep{10, 5}));
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true, .isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("transform-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptTransform, AScriptInterpolatesWhatItMovesInFixedUpdate) {
    Scripts scripts;
    ASSERT_EQ(scripts.run(R"(
        engine.ecs.system("test.Spawn", {
            phase = "Startup",
            query = {},
            run = function(ctx)
                ctx.commands:spawn({ ["aylo.Transform"] = {}, ["aylo.TransformInterpolation"] = {} })
            end,
        })
        engine.ecs.system("test.Move", {
            phase = "FixedUpdate",
            query = { write = { "aylo.Transform" } },
            run = function(ctx)
                for entity, transform in ctx.query do
                    transform.position = { x = transform.position.x + 10, y = 0 }
                end
            end,
        })
    )"),
              "");
    for (int frame = 0; frame < 3; ++frame) {  // no tick, tick 1, then halfway to the next tick
        scripts.app.runFrame();
    }

    int seen = 0;
    const aylo::ecs::QueryState<const Transform, const GlobalTransform, const TransformInterpolation> state{
        scripts.app.getWorld().getComponents()};
    aylo::ecs::Query<const Transform, const GlobalTransform, const TransformInterpolation> ships{state, 0};
    for (auto [entity, transform, global, interpolation] : ships) {
        EXPECT_EQ(transform.position.x, 10.0F);  // the simulation is untouched...
        EXPECT_EQ(global.position.x, 5.0F);      // ...and drawn halfway between the last two ticks
        ++seen;
    }
    EXPECT_EQ(seen, 1);
}
