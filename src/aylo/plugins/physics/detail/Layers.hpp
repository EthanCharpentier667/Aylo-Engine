/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Layers
*/

#pragma once

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Core.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdint>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Export.hpp"

namespace aylo::plugins::physics::detail {

/// @brief Each body's JPH::ObjectLayer, which carries everything collision filtering needs, since Jolt passes nothing
/// else to the filters: the collision categories the body belongs to (Collider::layer), the categories it collides with
/// (Collider::mask), whether it is static and whether it is a 3D body, which pick its broad phase tree.
/// @details Bits 0 to 14 hold the categories inverted, 15 to 29 the mask, 30 the static flag, 31 the 3D flag. A body
/// has at least one category, so one of bits 0 to 14 is clear and a layer is never JPH::cObjectLayerInvalid. Two
/// bodies collide when they are of the same family (2D or 3D) and each one's categories are in the other's mask,
/// unless both are static. Needs 32-bit object layers (object_layer_bits = "32" in the package configuration).
namespace layers {
static_assert(sizeof(JPH::ObjectLayer) == sizeof(std::uint32_t), "Collision layers need 32-bit Jolt object layers");

inline constexpr std::uint32_t kCategoryCount = 15;                               ///< Collision categories available.
inline constexpr std::uint32_t kAllCategories = (1U << kCategoryCount) - 1U;      ///< Every category's bit.
inline constexpr std::uint32_t kStaticBit = 1U << (2U * kCategoryCount);          ///< Set for a static body.
inline constexpr std::uint32_t kSpatialBit = 1U << ((2U * kCategoryCount) + 1U);  ///< Set for a 3D body.
static_assert(kAllCategories == kAllLayers, "Collider's kAllLayers must name the categories the encoding holds");

/// @return Whether categories and a mask fit in a layer: at least one category, and no bit beyond kAllCategories.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API bool isValid(std::uint32_t categories, std::uint32_t mask) noexcept;

/// @return The object layer of a body; bits beyond kAllCategories are dropped (check with isValid() first).
/// @param spatial Whether it is a 3D body.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API JPH::ObjectLayer encode(std::uint32_t categories, std::uint32_t mask,
                                                              bool isStatic, bool spatial = false) noexcept;

/// @return The collision categories a body belongs to.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API std::uint32_t categoriesOf(JPH::ObjectLayer layer) noexcept;

/// @return The collision categories a body collides with.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API std::uint32_t maskOf(JPH::ObjectLayer layer) noexcept;

/// @return Whether the body is static.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API bool isStatic(JPH::ObjectLayer layer) noexcept;

/// @return Whether the body is a 3D body.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API bool isSpatial(JPH::ObjectLayer layer) noexcept;

/// @return Whether two bodies can collide: same family, each one's categories in the other's mask, not both static.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API bool canCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) noexcept;
}  // namespace layers

/// @brief The broad phase layers: per family (2D, 3D), one tree for static bodies, one for moving ones. A family never
/// looks into the other's trees, so 2D and 3D bodies never meet, even at the same place.
namespace broadPhaseLayers {
inline constexpr JPH::BroadPhaseLayer kNonMoving{0};    ///< Static 2D bodies.
inline constexpr JPH::BroadPhaseLayer kMoving{1};       ///< Dynamic and kinematic 2D bodies.
inline constexpr JPH::BroadPhaseLayer kNonMoving3D{2};  ///< Static 3D bodies.
inline constexpr JPH::BroadPhaseLayer kMoving3D{3};     ///< Dynamic and kinematic 3D bodies.
inline constexpr std::uint32_t kCount = 4;              ///< Broad phase layers in use.
}  // namespace broadPhaseLayers

/// @brief Maps each object layer to its broad phase layer: its family and static flag pick the tree.
class AYLO_PLUGIN_PHYSICS_API BroadPhaseLayers final : public JPH::BroadPhaseLayerInterface {
  public:
    BroadPhaseLayers() = default;
    ~BroadPhaseLayers() override = default;
    BroadPhaseLayers(const BroadPhaseLayers& other) = delete;
    BroadPhaseLayers& operator=(const BroadPhaseLayers& other) = delete;
    BroadPhaseLayers(BroadPhaseLayers&& other) noexcept = delete;
    BroadPhaseLayers& operator=(BroadPhaseLayers&& other) noexcept = delete;

    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override;
    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override;
#endif
};

/// @brief Whether an object layer can collide with a broad phase layer: its family's trees only, and static bodies
/// skip the static tree.
class AYLO_PLUGIN_PHYSICS_API ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
  public:
    ObjectVsBroadPhaseFilter() = default;
    ~ObjectVsBroadPhaseFilter() override = default;
    ObjectVsBroadPhaseFilter(const ObjectVsBroadPhaseFilter& other) = delete;
    ObjectVsBroadPhaseFilter& operator=(const ObjectVsBroadPhaseFilter& other) = delete;
    ObjectVsBroadPhaseFilter(ObjectVsBroadPhaseFilter&& other) noexcept = delete;
    ObjectVsBroadPhaseFilter& operator=(ObjectVsBroadPhaseFilter&& other) noexcept = delete;

    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const override;
};

/// @brief Whether two object layers can collide: layers::canCollide().
class AYLO_PLUGIN_PHYSICS_API ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
  public:
    ObjectLayerPairFilter() = default;
    ~ObjectLayerPairFilter() override = default;
    ObjectLayerPairFilter(const ObjectLayerPairFilter& other) = delete;
    ObjectLayerPairFilter& operator=(const ObjectLayerPairFilter& other) = delete;
    ObjectLayerPairFilter(ObjectLayerPairFilter&& other) noexcept = delete;
    ObjectLayerPairFilter& operator=(ObjectLayerPairFilter&& other) noexcept = delete;

    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const override;
};

}  // namespace aylo::plugins::physics::detail
