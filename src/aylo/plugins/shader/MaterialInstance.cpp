/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialInstance
*/

#include "plugins/shader/MaterialInstance.hpp"

#include <string>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "plugins/shader/Material.hpp"

namespace aylo::plugins::shader {

MaterialInstance MaterialInstance::of(const contract::renderer::CompiledShader& shader) {
    return MaterialInstance{.values = shader.instanceDefaults};
}

MaterialInstance& MaterialInstance::set(const contract::renderer::CompiledShader& shader, const std::string& name,
                                        const MaterialValue& value) {
    writeInstanceValue(shader, values, name, value);
    return *this;
}

}  // namespace aylo::plugins::shader
