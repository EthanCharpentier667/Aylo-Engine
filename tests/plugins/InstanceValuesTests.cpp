/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InstanceValuesTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <string>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/shader/InstanceValues.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"

using aylo::core::asset::AssetId;
using aylo::plugins::shader::InstanceValueRequest;
using aylo::plugins::shader::InstanceValueRequests;
using aylo::plugins::shader::MaterialInstance;

namespace {
/// @brief A client App with the drawing plugins over the fake backend, and a material whose shader declares two
/// instance values: `glow` (default 0.5) and `tint` (default 1, 1, 1, 1).
class InstanceApp {
  public:
    InstanceApp() : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()} {
        _state->shaders = true;
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
        auto& resources = _app.getWorld().getResources();
        const AssetId shader = resources.getMut<aylo::plugins::shader::ShaderAssets>()->add(
            "glow",
            "instance uniform float glow = 0.5;\ninstance uniform vec4 tint = vec4(1.0);\n"
            "void fragment() { COLOR *= tint * glow; }\n");
        material = resources.getMut<aylo::plugins::shader::MaterialAssets>()->add("glow", {.shader = shader});
    }

    /// @brief Asks for @p name of @p entity to become @p value, as `aylo.shader.setInstanceValue` does.
    void request(aylo::ecs::Entity entity, const std::string& name, const aylo::plugins::shader::MaterialValue& value) {
        _app.getWorld().getResources().getMut<InstanceValueRequests>()->requests.push_back(
            InstanceValueRequest{.entity = entity, .material = material, .name = name, .value = value});
    }

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] const MaterialInstance* instanceOf(aylo::ecs::Entity entity) const {
        return _app.getWorld().getComponents().get<MaterialInstance>(entity);
    }

    AssetId material;  ///< The material of the glow shader.

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app{aylo::ecs::RunMode::kClient};               ///< The App under test.
};
}  // namespace

TEST(InstanceValues, GivesAnEntityWithoutAnInstanceTheShadersDefaultsAndEveryValueOfTheFrame) {
    InstanceApp test;
    const aylo::ecs::Entity entity = test.app().getWorld().spawn();
    test.request(entity, "tint", glm::vec4{1.0F, 0.0F, 0.0F, 1.0F});
    test.request(entity, "glow", 2.0F);
    test.app().runFrame();

    const MaterialInstance* instance = test.instanceOf(entity);
    ASSERT_NE(instance, nullptr);
    EXPECT_FLOAT_EQ(instance->values.at(0).x, 2.0F);  // glow, given second, did not undo tint
    EXPECT_FLOAT_EQ(instance->values.at(1).x, 1.0F);  // tint, at offset 16
    EXPECT_FLOAT_EQ(instance->values.at(1).y, 0.0F);
    EXPECT_TRUE(test.app().getWorld().getResources().get<InstanceValueRequests>()->requests.empty());
}

TEST(InstanceValues, ChangesOnlyTheValueGivenOfAnExistingInstance) {
    InstanceApp test;
    const aylo::ecs::Entity entity = test.app().getWorld().spawn();
    test.request(entity, "glow", 3.0F);
    test.app().runFrame();
    test.request(entity, "tint", glm::vec4{0.0F, 1.0F, 0.0F, 1.0F});
    test.app().runFrame();

    const MaterialInstance* instance = test.instanceOf(entity);
    ASSERT_NE(instance, nullptr);
    EXPECT_FLOAT_EQ(instance->values.at(0).x, 3.0F);
    EXPECT_FLOAT_EQ(instance->values.at(1).y, 1.0F);
}

TEST(InstanceValues, DropsAValueTheShaderDoesNotDeclareAndSaysSo) {
    InstanceApp test;
    const aylo::ecs::Entity entity = test.app().getWorld().spawn();
    test.request(entity, "missing", 1.0F);
    testing::internal::CaptureStderr();
    test.app().runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("aylo.shader.setInstanceValue"), std::string::npos) << log;
    const MaterialInstance* instance = test.instanceOf(entity);
    ASSERT_NE(instance, nullptr);  // given the defaults
    EXPECT_FLOAT_EQ(instance->values.at(0).x, 0.5F);
}

TEST(InstanceValues, DropsTheValuesOfAnEntityThatNoLongerExists) {
    InstanceApp test;
    const aylo::ecs::Entity entity = test.app().getWorld().spawn();
    test.app().getWorld().despawn(entity);
    test.request(entity, "glow", 1.0F);
    EXPECT_NO_THROW(test.app().runFrame());
    EXPECT_TRUE(test.app().getWorld().getResources().get<InstanceValueRequests>()->requests.empty());
}
