/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptAudioTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "FakeAudioDevice.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/audio/AudioPlugin.hpp"
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
