/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshCalls
*/

#include "plugins/mesh/MeshCalls.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iterator>
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

FindPartResult findPart(ecs::Res<MeshAssets> meshes, const FindPartArguments& arguments) {
    const MeshAssets::Entry* entry = meshes->get(arguments.mesh);
    if (entry == nullptr) {
        return FindPartResult{};
    }
    const auto found = std::ranges::find(entry->partNames, arguments.name.view());
    return FindPartResult{.part = found == entry->partNames.end()
                                      ? 0
                                      : static_cast<std::int32_t>(std::distance(entry->partNames.begin(), found)) + 1};
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
    app.addCall(std::string{kFindPartCall}, &findPart);
    addMeshForms(app.getPluginRegistry());
}

}  // namespace aylo::plugins::mesh
