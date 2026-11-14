/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshCalls
*/

#include "plugins/mesh/MeshCalls.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "plugins/mesh/MeshAssets.hpp"

namespace aylo::plugins::mesh {
namespace {
MeshResult loadMesh(ecs::ResMut<MeshAssets> meshes, const LoadMeshArguments& arguments) {
    return MeshResult{.mesh = meshes->load(std::filesystem::path{arguments.path.view()})};
}
}  // namespace

void addMeshForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "MeshForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addMeshForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addMeshCalls(ecs::App& app) {
    app.addCall(std::string{kLoadMeshCall}, &loadMesh);
    addMeshForms(app.getPluginRegistry());
}

}  // namespace aylo::plugins::mesh
