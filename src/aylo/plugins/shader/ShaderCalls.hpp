/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderCalls
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/shader/Export.hpp"

namespace aylo::plugins::shader {

inline constexpr std::string_view kLoadShaderCall = "aylo.shader.load";            ///< ShaderAssets::load().
inline constexpr std::string_view kAddMaterialCall = "aylo.shader.addMaterial";    ///< MaterialAssets::add().
inline constexpr std::string_view kSetValueCall = "aylo.shader.setMaterialValue";  ///< MaterialAssets::setValue().
inline constexpr std::string_view kSetTextureCall =
    "aylo.shader.setMaterialTexture";  ///< MaterialAssets::setTexture().
inline constexpr std::string_view kSetScreenMaterialsCall =
    "aylo.shader.setScreenMaterials";  ///< ScreenMaterials, replaced.

/// @brief The arguments of `aylo.shader.load`: reads and compiles a shader file.
struct LoadShaderArguments {
    ecs::String path;  ///< The file, relative to the game directory.
};
AYLO_ECS_CALL_DATA(LoadShaderArguments, "aylo.shader.LoadShaderArguments", path);

/// @brief The result of `aylo.shader.load`.
struct ShaderResult {
    core::asset::AssetId shader;  ///< The shader's id, for `aylo.shader.addMaterial`.
};
AYLO_ECS_CALL_DATA(ShaderResult, "aylo.shader.ShaderResult", shader);

/// @brief The arguments of `aylo.shader.addMaterial`: a material of @ref shader, with the shader's default values.
struct AddMaterialArguments {
    ecs::String name;             ///< What the material is known by.
    core::asset::AssetId shader;  ///< Its shader, from `aylo.shader.load`.
};
AYLO_ECS_CALL_DATA(AddMaterialArguments, "aylo.shader.AddMaterialArguments", name, shader);

/// @brief The result of `aylo.shader.addMaterial`.
struct MaterialResult {
    core::asset::AssetId material;  ///< The material's id, for a SpriteRenderer and the calls below.
};
AYLO_ECS_CALL_DATA(MaterialResult, "aylo.shader.MaterialResult", material);

/// @brief The arguments of `aylo.shader.setMaterialValue`: a float, or a vector of @ref size components.
struct SetMaterialValueArguments {
    core::asset::AssetId material;  ///< The material, from `aylo.shader.addMaterial`.
    ecs::String name;               ///< The `uniform`, as declared in the shader.
    std::uint32_t size{1};          ///< 1: a float (or a whole number, for an `int`); 2 to 4: a vec2 to a vec4.
    float x{0.0F};                  ///< The float, or the first component.
    float y{0.0F};                  ///< The second component.
    float z{0.0F};                  ///< The third component.
    float w{0.0F};                  ///< The fourth component.
};
AYLO_ECS_CALL_DATA(SetMaterialValueArguments, "aylo.shader.SetMaterialValueArguments", material, name, size, x, y, z,
                   w);

/// @brief The arguments of `aylo.shader.setMaterialTexture`.
struct SetMaterialTextureArguments {
    core::asset::AssetId material;  ///< The material, from `aylo.shader.addMaterial`.
    ecs::String name;               ///< The `uniform sampler2D`, as declared in the shader.
    core::asset::AssetId texture;   ///< The texture, from `aylo.backend.loadTexture`.
};
AYLO_ECS_CALL_DATA(SetMaterialTextureArguments, "aylo.shader.SetMaterialTextureArguments", material, name, texture);

/// @brief The arguments of `aylo.shader.setScreenMaterials`: the screen effects, replacing the previous ones.
struct SetScreenMaterialsArguments {
    std::vector<core::asset::AssetId>
        materials;  ///< From `aylo.shader.addMaterial`, in the order they run; empty: none.
};
AYLO_ECS_CALL_DATA(SetScreenMaterialsArguments, "aylo.shader.SetScreenMaterialsArguments", materials);

/// @brief Registers the shader engine calls on @p app, for scripts: `aylo.shader.load`, `aylo.shader.addMaterial`,
/// `aylo.shader.setMaterialValue`, `aylo.shader.setMaterialTexture` and `aylo.shader.setScreenMaterials`. They throw a
/// ShaderException naming what is wrong (an unknown material, a size that is not 1 to 4). Called by the ShaderPlugin.
void addShaderCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
AYLO_PLUGIN_SHADER_API void addShaderForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::shader
