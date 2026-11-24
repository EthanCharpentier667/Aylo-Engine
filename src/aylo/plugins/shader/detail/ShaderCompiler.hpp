/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderCompiler
*/

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "plugins/shader/Export.hpp"
#include "plugins/shader/detail/ShaderAssembler.hpp"
#include "plugins/shader/detail/ShaderIncludes.hpp"

namespace aylo::plugins::shader::detail {

/// @brief The targets of a shader that draws 2D things (neither `#pragma screen` nor `#pragma mesh`).
inline constexpr std::array<contract::renderer::ShaderTarget, 4> kDrawingTargets{
    contract::renderer::ShaderTarget::kSprite, contract::renderer::ShaderTarget::kGlyph,
    contract::renderer::ShaderTarget::kParticle, contract::renderer::ShaderTarget::kShape};

/// @brief The target of a screen effect (`#pragma screen`): it draws nothing else.
inline constexpr std::array<contract::renderer::ShaderTarget, 1> kScreenTargets{
    contract::renderer::ShaderTarget::kScreen};

/// @brief The target of a 3D shader (`#pragma mesh`): static meshes only.
inline constexpr std::array<contract::renderer::ShaderTarget, 1> kMeshTargets{contract::renderer::ShaderTarget::kMesh};

/// @return The SPIR-V 1.6 (Vulkan semantics: the engine's intermediate format, which every backend uses or translates)
/// of @p glsl, a complete GLSL 450 @p stage (assembleShader()).
/// @param what Names the stage in messages: the shader, its target and stage.
/// @throws exceptions::ShaderException If glslang rejects it: its errors, each with its line in the shader.
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::vector<std::uint32_t> compileStage(std::string_view glsl, ShaderStage stage,
                                                                             std::string_view what);

/// @return @p source, a shader in the engine's dialect, compiled for @p targets (empty: kScreenTargets for a shader
/// that says `#pragma screen`, kMeshTargets for `#pragma mesh`, kDrawingTargets for any other): their SPIR-V, and what
/// the parser found (the material and instance values with their defaults, the textures). Ready for
/// IRenderer::createShader(): it passes contract::renderer::validate().
/// @details glslang's process-wide state is set up by the first compilation, once, and torn down at exit: glslang
/// wants it exactly once per process.
/// @param name Names the shader in messages: its file, or its name.
/// @param read Reads the game files its `#include`s name (expandIncludes()); empty: only the engine's libraries.
/// @param included Filled, if given, with the files it included (their names, the engine's libraries too), for a
/// reload to follow them.
/// @throws exceptions::ShaderException If an include fails, the parser cannot read a declaration or glslang rejects a
/// stage: a line in an included file is named with that file.
[[nodiscard]] AYLO_PLUGIN_SHADER_API contract::renderer::CompiledShader compileShader(
    std::string_view source, std::string_view name, std::span<const contract::renderer::ShaderTarget> targets = {},
    const IncludeReader& read = {}, std::vector<std::string>* included = nullptr);

}  // namespace aylo::plugins::shader::detail
