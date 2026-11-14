/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialInspector
*/

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "AssetKind.hpp"
#include "FieldEditor.hpp"
#include "Panels.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "editor/EditorState.hpp"
#include "editor/gui/Text.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialFile.hpp"
#include "plugins/shader/ShaderAssets.hpp"

namespace aylo::editor::panels {

namespace {
namespace shader = plugins::shader;
namespace cr = contract::renderer;

/// @brief The game's shaders, materials and textures; null members when the game has no ShaderPlugin.
struct MaterialResources {
    shader::ShaderAssets* shaders{nullptr};
    shader::MaterialAssets* materials{nullptr};
    plugins::graphics::TextureAssets* textures{nullptr};

    [[nodiscard]] bool complete() const noexcept {
        return shaders != nullptr && materials != nullptr && textures != nullptr;
    }
};

/// @return The resources a material needs, from @p state's world.
MaterialResources resourcesOf(const EditorState& state) {
    ecs::Resources& resources = state.world->getResources();
    auto shaders = resources.getMut<shader::ShaderAssets>();
    auto materials = resources.getMut<shader::MaterialAssets>();
    auto textures = resources.getMut<plugins::graphics::TextureAssets>();
    return MaterialResources{.shaders = shaders ? &*shaders : nullptr,
                             .materials = materials ? &*materials : nullptr,
                             .textures = textures ? &*textures : nullptr};
}

/// @return Whether a value named @p name holds a colour, so it is edited with a colour picker.
bool isColour(std::string_view name) {
    std::string lowered{name};
    std::ranges::transform(lowered, lowered.begin(), [](unsigned char letter) { return std::tolower(letter); });
    return lowered.contains("color") || lowered.contains("colour") || lowered.contains("tint");
}

/// @return The value @p declared has in @p material, or the shader's default.
shader::MaterialValue currentValue(const shader::Material& material, const cr::CompiledShader& compiled,
                                   const cr::ShaderValue& declared) {
    const auto set = std::ranges::find(material.values, declared.name, &shader::Material::Value::name);
    if (set != material.values.end()) {
        return set->value;
    }
    const auto read = [&compiled, &declared]<typename T>(T value) {
        if (declared.offset + sizeof(T) <= compiled.materialDefaults.size()) {
            std::memcpy(&value, std::next(compiled.materialDefaults.data(), declared.offset), sizeof(T));
        }
        return value;
    };
    switch (declared.type) {
        case cr::ShaderValueType::kFloat:
            return read(0.0F);
        case cr::ShaderValueType::kVec2:
            return read(glm::vec2{0.0F});
        case cr::ShaderValueType::kVec3:
            return read(glm::vec3{0.0F});
        case cr::ShaderValueType::kVec4:
            return read(glm::vec4{0.0F});
        case cr::ShaderValueType::kInt:
            return read(std::int32_t{0});
    }
    return 0.0F;
}

/// @brief Shows a value with the widget of its type. @return Whether it changed.
bool editValue(const std::string& name, shader::MaterialValue& value) {
    constexpr float kSpeed = 0.01F;
    return std::visit(
        [&name](auto& held) {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, float>) {
                return ImGui::DragFloat(name.c_str(), &held, kSpeed);
            } else if constexpr (std::is_same_v<Held, std::int32_t>) {
                return ImGui::DragInt(name.c_str(), &held);
            } else if constexpr (std::is_same_v<Held, glm::vec2>) {
                return ImGui::DragFloat2(name.c_str(), glm::value_ptr(held), kSpeed);
            } else if constexpr (std::is_same_v<Held, glm::vec3>) {
                return isColour(name) ? ImGui::ColorEdit3(name.c_str(), glm::value_ptr(held))
                                      : ImGui::DragFloat3(name.c_str(), glm::value_ptr(held), kSpeed);
            } else {
                return isColour(name) ? ImGui::ColorEdit4(name.c_str(), glm::value_ptr(held))
                                      : ImGui::DragFloat4(name.c_str(), glm::value_ptr(held), kSpeed);
            }
        },
        value);
}

/// @return The asset file dropped on the last item, if it is of @p kind.
std::optional<std::string> droppedFile(AssetKind kind) {
    std::optional<std::string> dropped;
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetPayload)) {
            std::string path{static_cast<const char*>(payload->Data), static_cast<std::size_t>(payload->DataSize)};
            if (kindOf(path, false) == kind) {
                dropped = std::move(path);
            }
        }
        ImGui::EndDragDropTarget();
    }
    return dropped;
}

/// @return The id of the material shown, read from its file the first time; nothing if it cannot be.
std::optional<core::asset::AssetId> loadShown(EditorState& state, const MaterialResources& resources) {
    const core::asset::AssetId id = core::asset::AssetId::fromPath(state.inspectedMaterial);
    if (resources.materials->get(id) != nullptr) {
        return id;
    }
    try {
        return shader::loadMaterialFile(state.inspectedMaterial, *resources.shaders, *resources.materials,
                                        *resources.textures);
    } catch (const core::asset::exceptions::AssetLoadException& error) {
        state.materialError = error.what();
        return std::nullopt;
    }
}

/// @brief The shader row: its file, a drop target for another one.
void drawShader(EditorState& state, const MaterialResources& resources, core::asset::AssetId id,
                const shader::Material& material) {
    const shader::ShaderAssets::Entry* entry = resources.shaders->getEntry(material.shader);
    const std::string shown = entry != nullptr ? entry->path : std::string{"(none: drop a .glsl here)"};
    ImGui::TextUnformatted("Shader");
    ImGui::SameLine();
    ImGui::Button(std::format("{}  {}", toUtf8(iconOf(AssetKind::kShader)), shown).c_str());
    if (const std::optional<std::string> dropped = droppedFile(AssetKind::kShader)) {
        resources.materials->setShader(id, resources.shaders->load(*dropped));
        state.materialUnsaved = true;
    }
    if (entry != nullptr && !entry->error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 110, 110, 255));
        ImGui::TextWrapped("%s", entry->error.c_str());  // NOLINT(cppcoreguidelines-pro-type-vararg)
        ImGui::PopStyleColor();
    }
}

/// @brief The values and textures the shader declares, edited in place.
void drawValues(EditorState& state, const MaterialResources& resources, core::asset::AssetId id,
                const shader::Material& material, const cr::CompiledShader& compiled) {
    ImGui::SeparatorText("Values");
    if (compiled.materialValues.empty()) {
        gui::textDisabled("The shader declares no uniform.");
    }
    for (const cr::ShaderValue& declared : compiled.materialValues) {
        shader::MaterialValue value = currentValue(material, compiled, declared);
        if (editValue(declared.name, value)) {
            resources.materials->setValue(id, declared.name, value);
            state.materialUnsaved = true;
        }
    }
    if (!compiled.instanceValues.empty()) {
        std::string names;
        for (const cr::ShaderValue& declared : compiled.instanceValues) {
            names += std::format("{}{}", names.empty() ? "" : ", ", declared.name);
        }
        gui::textDisabled(std::format("Per entity (aylo.MaterialInstance): {}", names));
    }
    if (compiled.textures.empty()) {
        return;
    }
    ImGui::SeparatorText("Textures");
    for (const cr::ShaderTexture& declared : compiled.textures) {
        const auto set = std::ranges::find(material.textures, declared.name, &shader::Material::Texture::name);
        const plugins::graphics::TextureAssets::Entry* image =
            set != material.textures.end() ? resources.textures->get(set->texture) : nullptr;
        ImGui::TextUnformatted(declared.name.c_str());
        ImGui::SameLine();
        ImGui::PushID(declared.name.c_str());
        ImGui::Button(std::format("{}  {}", toUtf8(iconOf(AssetKind::kImage)),
                                  image != nullptr ? image->path : std::string{"(none: drop an image here)"})
                          .c_str());
        ImGui::PopID();
        if (const std::optional<std::string> dropped = droppedFile(AssetKind::kImage)) {
            resources.materials->setTexture(id, declared.name, resources.textures->load(*dropped));
            state.materialUnsaved = true;
        }
    }
}
}  // namespace

void drawMaterialInspector(EditorState& state) {
    gui::textDisabled(std::format("{}  Material: {}", toUtf8(iconOf(AssetKind::kMaterial)), state.inspectedMaterial));
    const MaterialResources resources = resourcesOf(state);
    if (!resources.complete()) {
        ImGui::TextWrapped(  // NOLINT(cppcoreguidelines-pro-type-vararg): a constant format.
            "The game has no aylo.Shader plugin: add it to engine.settings.plugins to edit materials.");
        return;
    }
    const std::optional<core::asset::AssetId> id = loadShown(state, resources);
    if (!id) {
        ImGui::TextWrapped("%s", state.materialError.c_str());  // NOLINT(cppcoreguidelines-pro-type-vararg)
        if (ImGui::Button("Read again")) {
            state.materialError.clear();
        }
        return;
    }
    const shader::Material material = *resources.materials->get(*id);  // a copy: editing replaces it
    drawShader(state, resources, *id, material);
    if (const cr::CompiledShader* compiled = resources.shaders->get(material.shader)) {
        drawValues(state, resources, *id, material, *compiled);
    }
    ImGui::Separator();
    ImGui::BeginDisabled(!state.materialUnsaved);
    if (ImGui::Button("Save (Ctrl+S)")) {
        saveMaterial(state);
    }
    ImGui::EndDisabled();
    if (state.materialUnsaved) {
        ImGui::SameLine();
        gui::textDisabled("changed: every drawing using it shows it already");
    }
}

void saveMaterial(EditorState& state) {
    const MaterialResources resources = resourcesOf(state);
    if (!resources.complete() || state.inspectedMaterial.empty() || state.assets.empty()) {
        return;
    }
    try {
        const shader::MaterialFile file =
            shader::describeMaterial(core::asset::AssetId::fromPath(state.inspectedMaterial), *resources.shaders,
                                     *resources.materials, *resources.textures);
        std::ofstream out{state.assets / state.inspectedMaterial, std::ios::binary | std::ios::trunc};
        out << file.write();
        if (!out) {
            state.status = std::format("Could not write {}", state.inspectedMaterial);
            return;
        }
    } catch (const core::asset::exceptions::AssetLoadException& error) {
        state.status = error.what();
        return;
    }
    state.materialUnsaved = false;
    state.status = std::format("Saved {}", state.inspectedMaterial);
}

}  // namespace aylo::editor::panels
