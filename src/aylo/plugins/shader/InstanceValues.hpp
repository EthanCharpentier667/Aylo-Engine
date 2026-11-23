/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InstanceValues
*/

#pragma once

#include <string>
#include <vector>

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/shader/Export.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialInstance.hpp"

namespace aylo::plugins::shader {

class MaterialAssets;
class ShaderAssets;

/// @brief One `instance uniform` value a script gave an entity (`aylo.shader.setInstanceValue`).
struct InstanceValueRequest {
    ecs::Entity entity;             ///< Whose MaterialInstance it goes in.
    core::asset::AssetId material;  ///< The material whose shader lays the values out.
    std::string name;               ///< The `instance uniform`, as declared.
    MaterialValue value;            ///< What it becomes.
};

/// @brief The values scripts gave entities since they were last applied, as the client resource
/// "aylo.InstanceValueRequests" the ShaderPlugin inserts. Its Render system `aylo.Shader.InstanceValues` writes them
/// into the entities' MaterialInstance, giving one (from the shader's defaults) to an entity without it, so that
/// several values given to a new entity in one frame all count.
struct InstanceValueRequests {
    std::vector<InstanceValueRequest> requests;  ///< In the order they were made.
};
AYLO_ECS_RESOURCE(InstanceValueRequests, "aylo.InstanceValueRequests");

/// @brief Writes @p requests into the entities' MaterialInstance, in their order: patched in place when the entity has
/// one, else one added through @p commands from its shader's defaults with every value given to it. @p requests is
/// emptied. A request for an unknown material, a shader that failed or a value the shader does not declare (or not of
/// that type) is dropped and said on the error output; one for an entity that no longer exists is dropped.
AYLO_PLUGIN_SHADER_API void applyInstanceValues(InstanceValueRequests& requests, const ShaderAssets& shaders,
                                                const MaterialAssets& materials,
                                                ecs::Query<MaterialInstance>& instances, ecs::Commands& commands);

}  // namespace aylo::plugins::shader
