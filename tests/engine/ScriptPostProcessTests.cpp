/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptPostProcessTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
/// @brief A client App with the GraphicsPlugin over the fake backend, and a script host on it.
struct ClientScripts {
    std::shared_ptr<aylo::test::plugins::FakeBackendState> state{
        std::make_shared<aylo::test::plugins::FakeBackendState>()};
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ClientScripts() {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(aylo::plugins::input::InputPlugin{});
        app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{aylo::test::plugins::makeFakeRegistry(state),
                                                              aylo::test::plugins::makeFakeSettings()});
        app.addPlugin(aylo::plugins::shader::ShaderPlugin{});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("post-process-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptPostProcess, AScriptTurnsOnBloomAndATonemapAndTheRendererGetsThem) {
    ClientScripts script;
    ASSERT_EQ(script.run(R"(
        engine.call("aylo.backend.setPostProcess", {
            bloom = true, bloomThreshold = 0.9, bloomKnee = 0.3, bloomIntensity = 0.5, tonemap = "aces", exposure = 1.2,
        })
    )"),
              "");
    script.app.runFrame();

    ASSERT_FALSE(script.state->postProcesses.empty());
    const aylo::contract::renderer::PostProcessSettings& settings = script.state->postProcesses.back();
    EXPECT_TRUE(settings.bloom.enabled);
    EXPECT_FLOAT_EQ(settings.bloom.threshold, 0.9F);
    EXPECT_FLOAT_EQ(settings.bloom.softKnee, 0.3F);
    EXPECT_FLOAT_EQ(settings.bloom.intensity, 0.5F);
    EXPECT_EQ(settings.tonemap, aylo::contract::renderer::Tonemap::kAces);
    EXPECT_FLOAT_EQ(settings.exposure, 1.2F);
}

TEST(ScriptPostProcess, ACallReplacesEverySettingSoWhatIsLeftOutGoesBackToItsDefault) {
    ClientScripts script;
    ASSERT_EQ(script.run(R"(
        engine.call("aylo.backend.setPostProcess", { bloom = true, tonemap = "reinhard", exposure = 2 })
        engine.call("aylo.backend.setPostProcess", { exposure = 0.5 })
    )"),
              "");
    const auto& postProcess = script.app.getWorld().getResources().require<aylo::plugins::graphics::PostProcess>();
    EXPECT_FALSE(postProcess.settings.bloom.enabled);
    EXPECT_EQ(postProcess.settings.tonemap, aylo::contract::renderer::Tonemap::kNone);
    EXPECT_FLOAT_EQ(postProcess.settings.exposure, 0.5F);
}

TEST(ScriptPostProcess, AnUnknownTonemapIsAnErrorThatNamesTheChoices) {
    ClientScripts script;
    ASSERT_EQ(script.run(R"lua(
        local ok, message = pcall(engine.call, "aylo.backend.setPostProcess", { tonemap = "filmic" })
        assert(not ok, "an unknown tonemap should fail")
        assert(string.find(message,
            "engine call 'aylo.backend.setPostProcess': Unknown tonemap 'filmic' (expected 'none', 'reinhard' or 'aces')",
            1, true), message)
    )lua"),
              "");
    const auto& postProcess = script.app.getWorld().getResources().require<aylo::plugins::graphics::PostProcess>();
    EXPECT_EQ(postProcess.settings, aylo::contract::renderer::PostProcessSettings{});
}
