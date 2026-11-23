/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderCalls
*/

#include "plugins/shader/ShaderCalls.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <string>
#include <string_view>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/InstanceValues.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialFile.hpp"
#include "plugins/shader/RenderStateNames.hpp"
#include "plugins/shader/ScreenMaterials.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

namespace aylo::plugins::shader {
namespace {
/// @return Whether the shader of @p material declares the `uniform` @p name as an `int`.
[[nodiscard]] bool isInt(const ShaderAssets& shaders, const MaterialAssets& materials,
                         const core::asset::AssetId material, const std::string_view name) {
    const Material* found = materials.get(material);
    const contract::renderer::CompiledShader* shader = found != nullptr ? shaders.get(found->shader) : nullptr;
    if (shader == nullptr) {
        return false;
    }
    const auto declared = std::ranges::find(shader->materialValues, name, &contract::renderer::ShaderValue::name);
    return declared != shader->materialValues.end() && declared->type == contract::renderer::ShaderValueType::kInt;
}

/// @return Whether the shader of @p material declares the `instance uniform` @p name as an `int`.
[[nodiscard]] bool isInstanceInt(const ShaderAssets& shaders, const MaterialAssets& materials,
                                 const core::asset::AssetId material, const std::string_view name) {
    const Material* found = materials.get(material);
    const contract::renderer::CompiledShader* shader = found != nullptr ? shaders.get(found->shader) : nullptr;
    if (shader == nullptr) {
        return false;
    }
    const auto declared = std::ranges::find(shader->instanceValues, name, &contract::renderer::ShaderValue::name);
    return declared != shader->instanceValues.end() && declared->type == contract::renderer::ShaderValueType::kInt;
}

/// @return The value @p arguments describe; a script has one kind of number, so it is an `int` if @p integer says so
/// and it is whole.
/// @throws exceptions::ShaderException If its size is not 1 to 4.
template <typename Arguments>
[[nodiscard]] MaterialValue readValue(const Arguments& arguments, const bool integer) {
    switch (arguments.size) {
        case 1:
            if (integer && std::trunc(arguments.x) == arguments.x) {
                return static_cast<std::int32_t>(arguments.x);
            }
            return arguments.x;
        case 2:
            return glm::vec2{arguments.x, arguments.y};
        case 3:
            return glm::vec3{arguments.x, arguments.y, arguments.z};
        case 4:
            return glm::vec4{arguments.x, arguments.y, arguments.z, arguments.w};
        default:
            throw exceptions::ShaderException{
                std::format("A material value has 1 to 4 components, not {}", arguments.size)};
    }
}

ShaderResult loadShader(ecs::ResMut<ShaderAssets> shaders, const LoadShaderArguments& arguments) {
    return ShaderResult{.shader = shaders->load(std::filesystem::path{arguments.path.view()})};
}

MaterialResult addMaterial(ecs::ResMut<MaterialAssets> materials, const AddMaterialArguments& arguments) {
    return MaterialResult{.material =
                              materials->add(std::string{arguments.name.view()}, Material{.shader = arguments.shader})};
}

MaterialResult loadMaterial(ecs::ResMut<ShaderAssets> shaders, ecs::ResMut<MaterialAssets> materials,
                            ecs::ResMut<graphics::TextureAssets> textures, const LoadMaterialArguments& arguments) {
    return MaterialResult{
        .material = loadMaterialFile(std::filesystem::path{arguments.path.view()}, *shaders, *materials, *textures)};
}

MaterialResult setMaterialValue(ecs::Res<ShaderAssets> shaders, ecs::ResMut<MaterialAssets> materials,
                                const SetMaterialValueArguments& arguments) {
    const bool integer = isInt(*shaders, *materials, arguments.material, arguments.name.view());
    if (!materials->setValue(arguments.material, std::string{arguments.name.view()}, readValue(arguments, integer))) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.material.value)};
    }
    return MaterialResult{.material = arguments.material};
}

MaterialResult setMaterialTexture(ecs::ResMut<MaterialAssets> materials, const SetMaterialTextureArguments& arguments) {
    if (!materials->setTexture(arguments.material, std::string{arguments.name.view()}, arguments.texture)) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.material.value)};
    }
    return MaterialResult{.material = arguments.material};
}

void setInstanceValue(ecs::Res<ShaderAssets> shaders, ecs::Res<MaterialAssets> materials,
                      ecs::ResMut<InstanceValueRequests> requests, const SetInstanceValueArguments& arguments) {
    if (materials->get(arguments.material) == nullptr) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.material.value)};
    }
    const bool integer = isInstanceInt(*shaders, *materials, arguments.material, arguments.name.view());
    requests->requests.push_back(InstanceValueRequest{.entity = arguments.entity,
                                                      .material = arguments.material,
                                                      .name = std::string{arguments.name.view()},
                                                      .value = readValue(arguments, integer)});
}

/// @return The value named @p name of a render state part, through @p named; nothing for an empty name.
/// @throws exceptions::ShaderException If @p name names none.
template <typename Named>
auto readRenderPart(std::string_view name, std::string_view part, Named named) -> decltype(named(name)) {
    if (name.empty()) {
        return std::nullopt;
    }
    auto value = named(name);
    if (!value) {
        throw exceptions::ShaderException{std::format("'{}' is not a render state {}", name, part)};
    }
    return value;
}

MaterialResult setMaterialRenderState(ecs::ResMut<MaterialAssets> materials,
                                      const SetMaterialRenderStateArguments& arguments) {
    RenderStateOverride state;
    state.cull = readRenderPart(arguments.cull.view(), "cull", &cullModeNamed);
    state.blend = readRenderPart(arguments.blend.view(), "blend", &blendNamed);
    state.depth = readRenderPart(arguments.depth.view(), "depth", &depthModeNamed);
    if (!std::isnan(arguments.depthBias)) {
        state.depthBias = arguments.depthBias;
    }
    if (!std::isnan(arguments.depthBiasSlope)) {
        state.depthBiasSlope = arguments.depthBiasSlope;
    }
    if (!materials->setRenderState(arguments.material, state)) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.material.value)};
    }
    return MaterialResult{.material = arguments.material};
}

MaterialResult setMaterialNextPass(ecs::ResMut<MaterialAssets> materials,
                                   const SetMaterialNextPassArguments& arguments) {
    if (arguments.nextPass != core::asset::AssetId{} && materials->get(arguments.nextPass) == nullptr) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.nextPass.value)};
    }
    if (arguments.nextPass == arguments.material) {
        throw exceptions::ShaderException{"A material cannot be its own next pass"};
    }
    if (!materials->setNextPass(arguments.material, arguments.nextPass)) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.material.value)};
    }
    return MaterialResult{.material = arguments.material};
}

void setScreenMaterials(ecs::Res<MaterialAssets> materials, ecs::ResMut<ScreenMaterials> screen,
                        const SetScreenMaterialsArguments& arguments) {
    for (const core::asset::AssetId material : arguments.materials) {
        if (materials->get(material) == nullptr) {
            throw exceptions::ShaderException{std::format("No material has the id {}", material.value)};
        }
    }
    screen->materials = arguments.materials;
}
}  // namespace

void addShaderForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "ShaderForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addShaderForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addShaderCalls(ecs::App& app) {
    addShaderForms(app.getPluginRegistry());
    app.addCall(std::string{kLoadShaderCall}, &loadShader);
    app.addCall(std::string{kAddMaterialCall}, &addMaterial);
    app.addCall(std::string{kLoadMaterialCall}, &loadMaterial);
    app.addCall(std::string{kSetValueCall}, &setMaterialValue);
    app.addCall(std::string{kSetTextureCall}, &setMaterialTexture);
    app.addCall(std::string{kSetRenderStateCall}, &setMaterialRenderState);
    app.addCall(std::string{kSetNextPassCall}, &setMaterialNextPass);
    app.addCall(std::string{kSetInstanceValueCall}, &setInstanceValue);
    app.addCall(std::string{kSetScreenMaterialsCall}, &setScreenMaterials);
}

}  // namespace aylo::plugins::shader
