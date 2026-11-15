/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptAudioTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "FakeAudioDevice.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
/// @brief A client App with the audio plugin over the fake device, and a script host on it.
struct ScriptAudio {
    std::shared_ptr<aylo::test::plugins::FakeAudioState> state{std::make_shared<aylo::test::plugins::FakeAudioState>()};
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ScriptAudio() {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(aylo::plugins::audio::AudioPlugin{aylo::test::plugins::makeFakeAudioRegistry(state),
                                                        aylo::test::plugins::makeFakeAudioSettings()});
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("audio-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptAudio, AScriptSchedulesALoopingVoiceAndControlsIt) {
    ScriptAudio script;
    script.app.runFrame();  // the device is created at startup
    script.state->time = 2.5;
    ASSERT_EQ(script.run(R"(
        local kick = engine.call("aylo.audio.loadSound", { path = "kick.wav" })
        assert(engine.call("aylo.audio.getTime", {}) == 2.5, "the audio clock")
        local voice = engine.call("aylo.audio.playSound", {
            sound = kick, bus = "effects", startAt = 3.0, loop = true, lowpass = 800,
        })
        assert(voice ~= 0)
        engine.call("aylo.audio.setVolume", { voice = voice, volume = 0.4 })
        engine.call("aylo.audio.setFilter", { voice = voice, lowpass = 300 })

        local other = engine.call("aylo.audio.playSound", { sound = kick, loop = true })
        assert(other ~= voice)
        engine.call("aylo.audio.stopVoice", { voice = other, fadeOut = 0.25 })
    )"),
              "");

    script.app.runFrame();

    ASSERT_EQ(script.state->plays.size(), 2U);
    const auto& play = script.state->plays.at(0).params;
    EXPECT_TRUE(play.loop);
    EXPECT_DOUBLE_EQ(play.startAt, 3.0);
    EXPECT_FLOAT_EQ(play.lowpass, 800.0F);
    EXPECT_FLOAT_EQ(script.state->volumes.at(0), 0.4F);
    EXPECT_FLOAT_EQ(script.state->lowpasses.at(0), 300.0F);
    EXPECT_TRUE(script.state->playing.contains(0));
    EXPECT_FALSE(script.state->playing.contains(1));
    ASSERT_EQ(script.state->stops.size(), 1U);
    EXPECT_FLOAT_EQ(script.state->stops.at(0).second, 0.25F);

    // A voice that is gone is ignored, not an error.
    ASSERT_EQ(script.run(R"(engine.call("aylo.audio.setVolume", { voice = 1, volume = 1 }))"), "");
    script.app.runFrame();
}

// The camera of the manual (engine-calls.md, "Sound in 3D"), and its system setting the listener, as written there.
constexpr const char* kManualCameraSpawn = R"(
ctx.commands:spawn({
	["aylo.Transform3D"] = { position = vector.create(0, 1.7, 0) },
	["aylo.Camera3D"] = {},
	["aylo.AudioListener"] = {},
})
)";
constexpr const char* kManualEars = R"(
local hit = engine.call("aylo.audio.loadSound", { path = "sounds/hit.wav" })

engine.ecs.system("game.Ears", {
	phase = "Update",
	side = "client",
	query = { read = { "aylo.Transform3D" }, with = { "aylo.Camera3D" } },
	calls = { "aylo.audio.setListener3D", "aylo.audio.playSound3D" },
	run = function(ctx)
		for _, eye in ctx.query do
			ctx.call("aylo.audio.setListener3D", { position = eye.position, rotation = eye.rotation })
		end
		-- a hit 4 m to the right of the world's origin, heard from the camera
		ctx.call("aylo.audio.playSound3D", {
			sound = hit, position = vector.create(4, 0, 0), minDistance = 2, maxDistance = 40,
		})
	end,
})
)";

TEST(ScriptAudio, TheManualsEarsHearA3DSoundFromTheCamera) {
    ScriptAudio script;
    static_cast<void>(script.app.getWorld().getComponents().registerType<aylo::plugins::graphics::Camera3D>());
    script.app.runFrame();
    const aylo::ecs::Entity camera = script.app.getWorld().spawn();
    script.app.getWorld().getComponents().add(camera,
                                              aylo::plugins::transform::Transform3D{.position = {0.0F, 1.7F, 0.0F}});
    script.app.getWorld().getComponents().add(camera, aylo::plugins::graphics::Camera3D{});
    ASSERT_EQ(script.run(kManualEars), "");

    script.app.runFrame();  // loads the sound
    script.app.runFrame();  // plays it

    // The hit, 4 m right and 1.7 m below the eye: on its right, 4.35 m away, at 2 / 4.35 of its volume.
    ASSERT_FALSE(script.state->plays.empty());
    const auto& play = script.state->plays.back().params;
    const float distance = std::sqrt((4.0F * 4.0F) + (1.7F * 1.7F));
    EXPECT_NEAR(play.pan, 4.0F / distance, 1e-3F);
    EXPECT_NEAR(play.volume, 2.0F / distance, 1e-3F);
}

TEST(ScriptAudio, ACameraWithAnAudioListenerIsThe3DListener) {
    ScriptAudio script;
    static_cast<void>(script.app.getWorld().getComponents().registerType<aylo::plugins::graphics::Camera3D>());
    ASSERT_EQ(script.run(std::string{R"(
        local hit = engine.call("aylo.audio.loadSound", { path = "sounds/hit.wav" })
        local spawned, played = false, false
        engine.ecs.system("game.Camera", {
            phase = "Update",
            side = "client",
            calls = { "aylo.audio.playSound3D" },
            run = function(ctx)
                if not spawned then
                    spawned = true
)"} + kManualCameraSpawn +
                         R"(
                elseif not played then
                    played = true
                    -- 6 m on the camera's left, beyond 3 times the minimum distance
                    ctx.call("aylo.audio.playSound3D", { sound = hit, position = vector.create(-6, 1.7, 0), minDistance = 2 })
                end
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 4; ++frame) {
        script.app.runFrame();
    }

    ASSERT_EQ(script.state->plays.size(), 1U);
    EXPECT_NEAR(script.state->plays[0].params.pan, -1.0F, 1e-3F);
    EXPECT_NEAR(script.state->plays[0].params.volume, 1.0F / 3.0F, 1e-3F);
}

TEST(ScriptAudio, PlaySound3DRefusesDistancesThatCannotBe) {
    ScriptAudio script;
    script.app.runFrame();
    const std::string error = script.run(R"(
        local hit = engine.call("aylo.audio.loadSound", { path = "hit.wav" })
        engine.call("aylo.audio.playSound3D", { sound = hit, position = vector.zero, minDistance = 10, maxDistance = 5 })
    )");
    EXPECT_NE(error.find("aylo.audio.playSound3D: the distances must be 0 < minDistance <= maxDistance"),
              std::string::npos)
        << error;
}
