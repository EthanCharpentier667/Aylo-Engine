/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Shader
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <string>
#include <vector>

#include "aylo/contract/renderer/Export.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"

namespace aylo::contract::renderer {

/// @brief A shader created by IRenderer::createShader(): a slot in the renderer's table and its generation, as
/// TextureId. Local to one renderer.
struct ShaderId {
    std::uint32_t index{0};       ///< Slot in the renderer's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "no shader".

    [[nodiscard]] bool operator==(const ShaderId& other) const = default;
};

/// @brief A material created by IRenderer::createMaterial(): a shader and its values. 0 is the default material, the
/// built-in shader of each drawing.
struct MaterialId {
    std::uint32_t index{0};       ///< Slot in the renderer's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "the default material".

    [[nodiscard]] bool operator==(const MaterialId& other) const = default;
};

/// @brief What a shader is drawn on: each has its own inputs, so a shader is compiled once per target it serves.
enum class ShaderTarget : std::uint8_t {
    kSprite,    ///< Sprites and the tiles of a tilemap: a textured quad.
    kGlyph,     ///< The glyphs of a Text: an MSDF atlas, read as distances.
    kParticle,  ///< Particles, of a CPU or a GPU simulation.
    kShape,     ///< Shapes: a quad over the shape's box, coloured from its signed distance.
    kScreen,  ///< The whole screen, once the world is drawn: a fullscreen triangle reading the scene (a screen effect).
    kMesh,    ///< The parts of a static mesh, in the 3D world (a `#pragma mesh` shader): unlit, with the depth test.
};

/// @brief Number of ShaderTarget values.
inline constexpr std::size_t kShaderTargetCount = 6;
static_assert(static_cast<std::size_t>(ShaderTarget::kMesh) + 1 == kShaderTargetCount,
              "kShaderTargetCount must count every ShaderTarget: kMesh is the last one");

/// @brief Which faces of a 3D material are not drawn.
enum class CullMode : std::uint8_t {
    kBack,   ///< Its back faces (seen from behind): closed models, the default.
    kFront,  ///< Its front faces: an inverted hull (outlines), the inside of a room.
    kNone,   ///< None: both sides drawn (leaves, cloth, planes seen from both sides).
};

/// @brief How a 3D material's pixels combine with what is behind them. The fragment's COLOR is premultiplied.
enum class MeshBlend : std::uint8_t {
    kOpaque,         ///< Replaces it; drawn first, in any order.
    kAlpha,          ///< Over it by COLOR.a (glass, smoke); drawn after the opaque, back to front.
    kPremultiplied,  ///< The same factors as kAlpha (COLOR is premultiplied): kept for materials written so.
    kAdditive,       ///< Added to it (fire, lasers, glows).
    kMultiply,       ///< Multiplies it (stains, tinted glass, shadows faked on a decal).
};

/// @brief How a 3D material uses the depth buffer.
enum class DepthMode : std::uint8_t {
    kTestWrite,    ///< Hidden by what is in front, and hides what is behind: the default of an opaque material.
    kTestNoWrite,  ///< Hidden by what is in front, hides nothing: the default of a blended one.
    kNoTest,       ///< Always drawn, hides nothing (an x-ray silhouette, a gizmo).
};

/// @brief How a 3D material is drawn, besides its code: from the shader's pragmas (`#pragma cull front`), and
/// replaced by a material (MaterialDesc::renderState).
struct RenderState {
    CullMode cull{CullMode::kBack};          ///< The faces not drawn.
    MeshBlend blend{MeshBlend::kOpaque};     ///< How it is blended.
    DepthMode depth{DepthMode::kTestWrite};  ///< How it uses depth.
    float depthBias{0.0F};       ///< Added to the depth, in steps of the depth buffer: negative pulls it closer.
    float depthBiasSlope{0.0F};  ///< Added to the depth times the surface's slope to the screen.

    [[nodiscard]] bool operator==(const RenderState& other) const = default;
};

/// @return Whether @p state is blended: drawn after the opaque draws, back to front.
[[nodiscard]] constexpr bool isTransparent(const RenderState& state) noexcept {
    return state.blend != MeshBlend::kOpaque;
}

/// @brief The type of a value a shader declares (`uniform float flash;`), as the shader reads it (std140 in a
/// material, std430 in an instance).
enum class ShaderValueType : std::uint8_t {
    kFloat,
    kVec2,
    kVec3,
    kVec4,
    kInt,
};

/// @brief Bytes of the values a drawing gives its own copy of (`instance uniform`): 4 vec4, carried with each sprite,
/// glyph, shape or particle draw so that one enemy can flash without its own material.
inline constexpr std::size_t kShaderInstanceBytes = 64;

/// @brief Textures a shader may read besides the drawing's own (`uniform sampler2D noise;`), given by its material.
inline constexpr std::size_t kShaderMaxTextures = 3;

/// @brief The `instance uniform` values of one drawing, as the shader reads them: kShaderInstanceBytes bytes.
using ShaderInstanceValues = std::array<glm::vec4, kShaderInstanceBytes / sizeof(glm::vec4)>;

/// @brief One value a shader declares, found by reflection.
struct ShaderValue {
    std::string name;                               ///< As written in the shader.
    ShaderValueType type{ShaderValueType::kFloat};  ///< Its type.
    std::uint32_t offset{0};                        ///< Bytes from the start of its block (material or instance).

    [[nodiscard]] bool operator==(const ShaderValue& other) const = default;
};

/// @brief One extra texture a shader declares, found by reflection.
struct ShaderTexture {
    std::string name;       ///< As written in the shader.
    std::uint32_t slot{0};  ///< Index in MaterialDesc::textures: 0 to kShaderMaxTextures - 1.

    [[nodiscard]] bool operator==(const ShaderTexture& other) const = default;
};

/// @brief A shader's compiled code for one target: SPIR-V, the format every backend starts from (Vulkan uses it as
/// is, others translate it, e.g. with SPIRV-Cross).
struct ShaderVariant {
    ShaderTarget target{ShaderTarget::kSprite};  ///< What it draws.
    std::vector<std::uint32_t> vertex;           ///< The vertex stage, SPIR-V words.
    std::vector<std::uint32_t> fragment;         ///< The fragment stage, SPIR-V words.
};

/// @brief A shader compiled by the shader plugin, given to IRenderer::createShader(): its code for each target it
/// serves, and what reflection found in it. Plain data: it says nothing of any graphics API.
struct AYLO_CONTRACT_RENDERER_API CompiledShader {
    std::string name;                         ///< For messages: its file, or the name it was added under.
    std::vector<ShaderVariant> variants;      ///< One per target, each target at most once.
    std::vector<ShaderValue> materialValues;  ///< `uniform` values, in the material's block.
    std::uint32_t materialSize{0};            ///< Bytes of the material's block.
    std::vector<std::byte> materialDefaults;  ///< The block filled with the declared defaults: materialSize bytes.
    std::vector<ShaderValue> instanceValues;  ///< `instance uniform` values, in a drawing's ShaderInstanceValues.
    ShaderInstanceValues instanceDefaults{};  ///< The declared defaults of the instance values.
    std::vector<ShaderTexture> textures;      ///< `uniform sampler2D` textures, in slot order.
    RenderState renderState;                  ///< How its kMesh variant is drawn (its pragmas); ignored by 2D targets.
    bool castsShadows{true};                  ///< kMesh: whether it casts shadows (`#pragma shadow off`: not).
    bool cutsOut{false};  ///< kMesh: whether it writes ALPHA_SCISSOR, so its shadow runs its fragment() too.

    /// @return The variant for @p target, or nullptr if the shader does not serve it.
    [[nodiscard]] const ShaderVariant* findVariant(ShaderTarget target) const noexcept;
};

/// @brief A shader and the values of its `uniform`s and textures, given to IRenderer::createMaterial(). Drawings that
/// share a material share its values; each still has its own `instance uniform` values.
struct MaterialDesc {
    ShaderId shader;                ///< From IRenderer::createShader().
    std::vector<std::byte> values;  ///< The material's block: CompiledShader::materialSize bytes.
    std::array<TextureId, kShaderMaxTextures> textures{};  ///< By slot; an unused slot stays a default TextureId.
    std::optional<RenderState> renderState;  ///< Replaces the shader's for its kMesh variant; none: the shader's.
};

/// @brief Checks @p shader can be created: at least one variant, no target twice, no empty stage, every value inside
/// its block, the defaults of the block's size, and texture slots below kShaderMaxTextures, each used once.
/// @throws std::invalid_argument Naming the shader and what is wrong.
AYLO_CONTRACT_RENDERER_API void validate(const CompiledShader& shader);

}  // namespace aylo::contract::renderer
