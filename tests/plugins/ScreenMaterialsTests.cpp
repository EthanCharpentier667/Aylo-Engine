/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenMaterialsTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/ScreenMaterials.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderHandles.hpp"

using aylo::contract::renderer::MaterialId;
using aylo::core::asset::AssetId;

namespace {
/// @brief A client App with the drawing plugins over the fake backend, which runs game shaders.
class ScreenApp {
  public:
    ScreenApp() : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()} {
        _state->shaders = true;
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
    }

    [[nodiscard]] aylo::ecs::Resources& resources() { return _app.getWorld().getResources(); }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }
    void runFrame() { _app.runFrame(); }

    /// @return A material of a new screen shader named @p name.
    AssetId addEffect(const char* name) {
        const AssetId shader = resources().getMut<aylo::plugins::shader::ShaderAssets>()->add(
            name, "#pragma screen\nvoid fragment() { COLOR.rgb = 1.0 - COLOR.rgb; }\n");
        return resources().getMut<aylo::plugins::shader::MaterialAssets>()->add(name, {.shader = shader});
    }

    /// @return What the renderer created the material @p id as.
    [[nodiscard]] MaterialId created(AssetId id) {
        const aylo::plugins::shader::ShaderHandles::Material* material =
            resources().get<aylo::plugins::shader::ShaderHandles>()->getMaterial(id);
        return material != nullptr ? material->material : MaterialId{};
    }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app{aylo::ecs::RunMode::kClient};               ///< The App under test.
};
}  // namespace

TEST(ScreenMaterials, TheRendererHasNoEffectByDefault) {
    ScreenApp app;
    app.runFrame();
    ASSERT_FALSE(app.state().screenMaterials.empty());
    EXPECT_TRUE(app.state().screenMaterials.back().empty());
}

TEST(ScreenMaterials, GivesTheRendererTheEffectsInTheirOrder) {
    ScreenApp app;
    const AssetId invert = app.addEffect("invert");
    const AssetId again = app.addEffect("again");
    app.resources().getMut<aylo::plugins::shader::ScreenMaterials>()->materials = {again, invert};
    app.runFrame();
    ASSERT_FALSE(app.state().screenMaterials.empty());
    const std::vector<MaterialId> expected{app.created(again), app.created(invert)};
    EXPECT_NE(expected.front(), MaterialId{});
    EXPECT_EQ(app.state().screenMaterials.back(), expected);
}

TEST(ScreenMaterials, SkipsAnEffectWhoseMaterialIsNotCreated) {
    ScreenApp app;
    const AssetId broken = app.resources().getMut<aylo::plugins::shader::ShaderAssets>()->add(
        "broken", "#pragma screen\nvoid fragment() { COLOR = nothing; }\n");
    const AssetId material =
        app.resources().getMut<aylo::plugins::shader::MaterialAssets>()->add("broken", {.shader = broken});
    const AssetId invert = app.addEffect("invert");
    app.resources().getMut<aylo::plugins::shader::ScreenMaterials>()->materials = {material, invert};
    app.runFrame();
    ASSERT_FALSE(app.state().screenMaterials.empty());
    EXPECT_EQ(app.state().screenMaterials.back(), std::vector<MaterialId>{app.created(invert)});
}
