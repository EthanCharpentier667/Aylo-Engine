/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InstanceValues
*/

#include "plugins/shader/InstanceValues.hpp"

#include <algorithm>
#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"

namespace aylo::plugins::shader {

namespace {

/// @return The shader of @p material, or nullptr (said on the error output) if there is none to lay values out.
const contract::renderer::CompiledShader* shaderOf(const ShaderAssets& shaders, const MaterialAssets& materials,
                                                   const InstanceValueRequest& request) {
    const Material* material = materials.get(request.material);
    const contract::renderer::CompiledShader* shader = material != nullptr ? shaders.get(material->shader) : nullptr;
    if (shader == nullptr) {
        std::cerr << std::format(
            "[Shader] aylo.shader.setInstanceValue: material {} has no compiled shader, '{}' is not set\n",
            request.material.value, request.name);
    }
    return shader;
}

}  // namespace

void applyInstanceValues(InstanceValueRequests& requests, const ShaderAssets& shaders, const MaterialAssets& materials,
                         ecs::Query<MaterialInstance>& instances, ecs::Commands& commands) {
    // The instances given to entities that have none yet, added once each, after every value they got.
    std::vector<std::pair<ecs::Entity, MaterialInstance>> added;
    for (const InstanceValueRequest& request : requests.requests) {
        const contract::renderer::CompiledShader* shader = shaderOf(shaders, materials, request);
        if (shader == nullptr) {
            continue;
        }
        MaterialInstance* instance = nullptr;
        if (auto found = instances.get(request.entity)) {
            instance = &*std::get<1>(*found);
        } else if (auto pending =
                       std::ranges::find(added, request.entity, &std::pair<ecs::Entity, MaterialInstance>::first);
                   pending != added.end()) {
            instance = &pending->second;
        } else {
            instance = &added.emplace_back(request.entity, MaterialInstance::of(*shader)).second;
        }
        try {
            instance->set(*shader, request.name, request.value);
        } catch (const std::exception& error) {
            std::cerr << std::format("[Shader] aylo.shader.setInstanceValue: {}\n", error.what());
        }
    }
    requests.requests.clear();
    for (auto& [entity, instance] : added) {
        commands.add(entity, instance);
    }
}

}  // namespace aylo::plugins::shader
