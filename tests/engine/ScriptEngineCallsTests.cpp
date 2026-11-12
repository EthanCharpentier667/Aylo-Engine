/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptEngineCallsTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/SpritePlugin.hpp"

namespace {
/// @brief A client App with the sprite and input plugins, and a script host on it.
struct ScriptApp {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ScriptApp() {
        app.addPlugin(aylo::plugins::sprite::SpritePlugin{});
        app.addPlugin(aylo::plugins::input::InputPlugin{});
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("engine-calls-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptEngineCalls, AScriptBuildsAtlasesFromTheSpritePlugin) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        local ship = engine.call("aylo.sprite.createAtlas", {
            name = "game.ship", texture = "ship.png", frameSize = { x = 48, y = 48 }, count = 5,
            spacing = { x = 17, y = 0 },
        })
        engine.call("aylo.sprite.addAnimation", { atlas = ship, name = "fly", direction = "pingPong" })

        engine.call("aylo.sprite.createAtlas", { name = "game.laser", texture = "laser.png" })
        for _, x in { 108, 189 } do
            engine.call("aylo.sprite.addFrame", {
                atlas = "game.laser", position = { x = x, y = 151 }, size = { x = 80, y = 16 },
            })
        end
        assert(engine.call("aylo.sprite.addFrame", { atlas = "game.laser", name = "end" }) == 2)
    )"),
              "");

    const auto& atlases = script.app.getWorld().getResources().require<aylo::plugins::sprite::AtlasAssets>();
    const auto* ship = atlases.get(aylo::core::asset::AssetId::fromPath("game.ship"));
    ASSERT_NE(ship, nullptr);
    EXPECT_EQ(ship->getFrames().size(), 5U);
    const auto* fly = ship->findAnimation(aylo::plugins::sprite::animationId("fly"));
    ASSERT_NE(fly, nullptr);
    EXPECT_EQ(fly->frames.size(), 5U);
    const auto* laser = atlases.get(aylo::core::asset::AssetId::fromPath("game.laser"));
    ASSERT_NE(laser, nullptr);
    ASSERT_EQ(laser->getFrames().size(), 3U);
    EXPECT_FLOAT_EQ(laser->getFrames().at(1).source.getX(), 189.0F);
}

TEST(ScriptEngineCalls, AScriptDeclaresInputActionsAndReadsThem) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        engine.call("aylo.input.addAction", { name = "move", type = "vector2" })
        engine.call("aylo.input.bindVector", { action = "move", up = "W", down = "S", left = "A", right = "D" })
        engine.call("aylo.input.addAction", { name = "shoot" })
        engine.call("aylo.input.bind", { action = "shoot", control = "MouseLeft" })

        local state = engine.call("aylo.input.read", { action = "shoot" })
        assert(state.held == false and state.value.x == 0, "nothing is pressed yet")

        local ok, message = pcall(engine.call, "aylo.input.bind", { action = "shoot", control = "Jump" })
        assert(not ok, "an unknown control should fail")
        assert(string.find(message, "engine call 'aylo.input.bind': Unknown control 'Jump'", 1, true), message)
    )"),
              "");

    auto& input = script.app.getWorld().getResources().require<aylo::plugins::input::Input>();
    EXPECT_NO_THROW(static_cast<void>(input.getAction("move")));
    EXPECT_NO_THROW(static_cast<void>(input.getAction("shoot")));
}
