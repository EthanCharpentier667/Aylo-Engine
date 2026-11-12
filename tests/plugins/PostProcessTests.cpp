/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostProcessTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "plugins/graphics/GraphicsResources.hpp"
#include "plugins/graphics/PostProcessCalls.hpp"
#include "plugins/graphics/exceptions/PostProcessCallException.hpp"

namespace renderer = aylo::contract::renderer;
namespace graphics = aylo::plugins::graphics;

TEST(PostProcessSettings, TheDefaultsChangeNothingOfThePicture) {
    const renderer::PostProcessSettings settings;
    EXPECT_FALSE(settings.bloom.enabled);
    EXPECT_EQ(settings.tonemap, renderer::Tonemap::kNone);
    EXPECT_FLOAT_EQ(settings.exposure, 1.0F);
    EXPECT_EQ(settings.sanitized(), settings);
}

TEST(PostProcessSettings, SanitizedBringsEveryValueIntoItsRange) {
    renderer::PostProcessSettings settings;
    settings.bloom = {.enabled = true, .threshold = -3.0F, .softKnee = 4.0F, .intensity = -1.0F};
    settings.exposure = -2.0F;
    const renderer::PostProcessSettings clean = settings.sanitized();
    EXPECT_TRUE(clean.bloom.enabled);
    EXPECT_FLOAT_EQ(clean.bloom.threshold, 0.0F);
    EXPECT_FLOAT_EQ(clean.bloom.softKnee, 1.0F);
    EXPECT_FLOAT_EQ(clean.bloom.intensity, 0.0F);
    EXPECT_FLOAT_EQ(clean.exposure, 0.0F);
}

TEST(PostProcessSettings, SanitizedReplacesNanAndInfinityByTheDefault) {
    renderer::PostProcessSettings settings;
    settings.bloom.threshold = std::numeric_limits<float>::quiet_NaN();
    settings.bloom.intensity = std::numeric_limits<float>::infinity();
    settings.exposure = std::numeric_limits<float>::quiet_NaN();
    const renderer::PostProcessSettings clean = settings.sanitized();
    EXPECT_FLOAT_EQ(clean.bloom.threshold, renderer::BloomSettings{}.threshold);
    EXPECT_FLOAT_EQ(clean.bloom.intensity, renderer::BloomSettings{}.intensity);
    EXPECT_FLOAT_EQ(clean.exposure, 1.0F);
}

TEST(PostProcessCalls, ArgumentsLeftOutAreTheDefaultSettings) {
    EXPECT_EQ(graphics::toPostProcessSettings(graphics::SetPostProcessArguments{}), renderer::PostProcessSettings{});
}

TEST(PostProcessCalls, TheArgumentsBecomeTheSettings) {
    graphics::SetPostProcessArguments arguments;
    arguments.bloom = true;
    arguments.bloomThreshold = 0.8F;
    arguments.bloomKnee = 0.2F;
    arguments.bloomIntensity = 0.6F;
    arguments.tonemap = aylo::ecs::String{"aces"};
    arguments.exposure = 1.4F;
    const renderer::PostProcessSettings settings = graphics::toPostProcessSettings(arguments);
    EXPECT_TRUE(settings.bloom.enabled);
    EXPECT_FLOAT_EQ(settings.bloom.threshold, 0.8F);
    EXPECT_FLOAT_EQ(settings.bloom.softKnee, 0.2F);
    EXPECT_FLOAT_EQ(settings.bloom.intensity, 0.6F);
    EXPECT_EQ(settings.tonemap, renderer::Tonemap::kAces);
    EXPECT_FLOAT_EQ(settings.exposure, 1.4F);
}

TEST(PostProcessCalls, EachTonemapHasItsName) {
    for (const auto& [name, tonemap] : {std::pair<const char*, renderer::Tonemap>{"none", renderer::Tonemap::kNone},
                                        {"reinhard", renderer::Tonemap::kReinhard},
                                        {"aces", renderer::Tonemap::kAces}}) {
        graphics::SetPostProcessArguments arguments;
        arguments.tonemap = aylo::ecs::String{name};
        EXPECT_EQ(graphics::toPostProcessSettings(arguments).tonemap, tonemap) << name;
    }
}

TEST(PostProcessCalls, AnUnknownTonemapNamesTheChoices) {
    graphics::SetPostProcessArguments arguments;
    arguments.tonemap = aylo::ecs::String{"filmic"};
    try {
        static_cast<void>(graphics::toPostProcessSettings(arguments));
        FAIL() << "an unknown tonemap should throw";
    } catch (const graphics::exceptions::PostProcessCallException& error) {
        EXPECT_NE(std::string{error.what()}.find("Unknown tonemap 'filmic' (expected 'none', 'reinhard' or 'aces')"),
                  std::string::npos)
            << error.what();
    }
}

TEST(PostProcessCalls, OutOfRangeArgumentsAreBroughtIntoRange) {
    graphics::SetPostProcessArguments arguments;
    arguments.bloomKnee = 7.0F;
    arguments.exposure = -1.0F;
    const renderer::PostProcessSettings settings = graphics::toPostProcessSettings(arguments);
    EXPECT_FLOAT_EQ(settings.bloom.softKnee, 1.0F);
    EXPECT_FLOAT_EQ(settings.exposure, 0.0F);
}

namespace {
/// @brief A client App with the GraphicsPlugin over the fake platform and renderer.
class BackendApp {
  public:
    BackendApp()
        : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
    }
    ~BackendApp() = default;
    BackendApp(const BackendApp&) = delete;
    BackendApp& operator=(const BackendApp&) = delete;
    BackendApp(BackendApp&&) = delete;
    BackendApp& operator=(BackendApp&&) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app;                                            ///< The App under test.
};
}  // namespace

TEST(PostProcessPlugin, InsertsThePostProcessResourceWithTheDefaults) {
    BackendApp app;
    const auto& postProcess = app.app().getWorld().getResources().require<graphics::PostProcess>();
    EXPECT_EQ(postProcess.settings, renderer::PostProcessSettings{});
}

TEST(PostProcessPlugin, HandsTheRendererTheSettingsOnceThenOnlyWhenTheyChange) {
    BackendApp app;
    app.app().runFrame();
    ASSERT_EQ(app.state().postProcesses.size(), 1U);
    EXPECT_EQ(app.state().postProcesses.front(), renderer::PostProcessSettings{});

    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().postProcesses.size(), 1U) << "nothing changed";

    renderer::PostProcessSettings bloom;
    bloom.bloom.enabled = true;
    bloom.tonemap = renderer::Tonemap::kReinhard;
    app.app().getWorld().getResources().getMut<graphics::PostProcess>()->settings = bloom;
    app.app().runFrame();
    ASSERT_EQ(app.state().postProcesses.size(), 2U);
    EXPECT_EQ(app.state().postProcesses.back(), bloom);
}
