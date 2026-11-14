/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** UiNode
*/

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"

namespace aylo::plugins::ui {

/// @brief How a node takes part in the layout.
enum class UiDisplay : std::uint8_t {
    kFlex,  ///< Laid out with its children.
    kNone,  ///< Neither laid out nor drawn, with its children.
};

/// @brief What a node is placed against.
enum class UiPosition : std::uint8_t {
    kRelative,  ///< Its place in the flow of its parent, moved by its insets.
    kAbsolute,  ///< The edges of its parent, out of the flow: what replaces anchors.
};

/// @brief The axis children are laid along.
enum class UiDirection : std::uint8_t {
    kColumn,         ///< Top to bottom.
    kColumnReverse,  ///< Bottom to top.
    kRow,            ///< Left to right.
    kRowReverse,     ///< Right to left.
};

/// @brief What children do when they overflow the main axis.
enum class UiWrap : std::uint8_t {
    kNoWrap,       ///< They stay on one line.
    kWrap,         ///< They go on to a new line.
    kWrapReverse,  ///< They go on to a new line, before the last one.
};

/// @brief How children share the main axis.
enum class UiJustify : std::uint8_t {
    kFlexStart,     ///< At its start.
    kCenter,        ///< In its middle.
    kFlexEnd,       ///< At its end.
    kSpaceBetween,  ///< Spread, the first and the last on the edges.
    kSpaceAround,   ///< Spread, with half a gap on the edges.
    kSpaceEvenly,   ///< Spread, with the same gap everywhere.
};

/// @brief How children sit on the cross axis.
enum class UiAlign : std::uint8_t {
    kAuto,          ///< As the parent says (alignSelf only).
    kFlexStart,     ///< At its start.
    kCenter,        ///< In its middle.
    kFlexEnd,       ///< At its end.
    kStretch,       ///< Filling it.
    kBaseline,      ///< On the first baseline.
    kSpaceBetween,  ///< Spread, lines on the edges (alignContent only).
    kSpaceAround,   ///< Spread, half a gap on the edges (alignContent only).
    kSpaceEvenly,   ///< Spread evenly (alignContent only).
};

/// @brief What happens to what overflows a node.
enum class UiOverflow : std::uint8_t {
    kVisible,  ///< It is drawn.
    kHidden,   ///< It is clipped.
    kScroll,   ///< It is clipped, and a UiScroll moves it.
};

/// @brief What a UiLength counts in.
enum class UiUnit : std::uint8_t {
    kAuto,     ///< Left to the layout: the default of every length.
    kPixels,   ///< View pixels.
    kPercent,  ///< A percentage of the parent's size.
};

/// @brief A size or a distance: a value and its unit, stored as two floats so that it reflects as a vec2.
struct UiLength {
    float value = 0.0F;  ///< The number of pixels, or the percentage.
    float unit = 0.0F;   ///< The UiUnit, as a float.

    /// @return @p value view pixels.
    [[nodiscard]] static constexpr UiLength px(const float value) noexcept {
        return UiLength{.value = value, .unit = static_cast<float>(UiUnit::kPixels)};
    }

    /// @return @p value percent of the parent's size.
    [[nodiscard]] static constexpr UiLength percent(const float value) noexcept {
        return UiLength{.value = value, .unit = static_cast<float>(UiUnit::kPercent)};
    }

    /// @return What the length counts in.
    [[nodiscard]] constexpr UiUnit getUnit() const noexcept { return static_cast<UiUnit>(static_cast<int>(unit)); }
};

}  // namespace aylo::plugins::ui

template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiDisplay>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a UiDisplay field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::ui::UiDisplay> {
    static constexpr std::array<std::string_view, 2> kNames{"flex", "none"};
};
static_assert(sizeof(aylo::plugins::ui::UiDisplay) == 1, "UiDisplay must match FieldType::kU8");
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiPosition>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a UiPosition field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::ui::UiPosition> {
    static constexpr std::array<std::string_view, 2> kNames{"relative", "absolute"};
};
static_assert(sizeof(aylo::plugins::ui::UiPosition) == 1, "UiPosition must match FieldType::kU8");
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiDirection>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a UiDirection field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::ui::UiDirection> {
    static constexpr std::array<std::string_view, 4> kNames{"column", "column-reverse", "row", "row-reverse"};
};
static_assert(sizeof(aylo::plugins::ui::UiDirection) == 1, "UiDirection must match FieldType::kU8");
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiWrap>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a UiWrap field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::ui::UiWrap> {
    static constexpr std::array<std::string_view, 3> kNames{"nowrap", "wrap", "wrap-reverse"};
};
static_assert(sizeof(aylo::plugins::ui::UiWrap) == 1, "UiWrap must match FieldType::kU8");
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiJustify>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a UiJustify field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::ui::UiJustify> {
    static constexpr std::array<std::string_view, 6> kNames{"start",         "center",       "end",
                                                            "space-between", "space-around", "space-evenly"};
};
static_assert(sizeof(aylo::plugins::ui::UiJustify) == 1, "UiJustify must match FieldType::kU8");
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiAlign>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a UiAlign field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::ui::UiAlign> {
    static constexpr std::array<std::string_view, 9> kNames{
        "auto", "start", "center", "end", "stretch", "baseline", "space-between", "space-around", "space-evenly"};
};
static_assert(sizeof(aylo::plugins::ui::UiAlign) == 1, "UiAlign must match FieldType::kU8");
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiOverflow>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a UiOverflow field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::ui::UiOverflow> {
    static constexpr std::array<std::string_view, 3> kNames{"visible", "hidden", "scroll"};
};
static_assert(sizeof(aylo::plugins::ui::UiOverflow) == 1, "UiOverflow must match FieldType::kU8");
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::ui::UiLength>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec2> {};
static_assert(sizeof(aylo::plugins::ui::UiLength) == 8 && alignof(aylo::plugins::ui::UiLength) == 4,
              "UiLength must match FieldType::kVec2");

namespace aylo::plugins::ui {

/// @brief Makes an entity a node of the user interface: every flexbox property Yoga has, and nothing else.
///
/// @details A node is an entity; its children are its ECS children (ecs::Parent) that are nodes too, and a node without
/// such a parent is a root, laid out over the whole view. What a node shows or does comes from the other components on
/// the same entity (UiBackground, UiText, UiButton...), so anything can be combined. The defaults are Yoga's: a column,
/// children stretched, no growing or shrinking. An absolute node with insets takes the place of an anchor.
/// @code
/// commands.spawn().add(UiNode{.position = UiPosition::kAbsolute, .right = UiLength::px(16.0F),
///                             .top = UiLength::px(16.0F), .width = UiLength::px(200.0F)});
/// @endcode
struct UiNode {
    UiDisplay display = UiDisplay::kFlex;          ///< Whether it is laid out at all.
    UiPosition position = UiPosition::kRelative;   ///< In the flow, or against its parent's edges.
    UiDirection direction = UiDirection::kColumn;  ///< The axis of its children.
    UiWrap wrap = UiWrap::kNoWrap;                 ///< Whether its children go on to new lines.
    UiJustify justify = UiJustify::kFlexStart;     ///< Its children on the main axis.
    UiAlign alignItems = UiAlign::kStretch;        ///< Its children on the cross axis.
    UiAlign alignSelf = UiAlign::kAuto;            ///< Itself on its parent's cross axis.
    UiAlign alignContent = UiAlign::kFlexStart;    ///< Its lines, when its children wrap.
    UiOverflow overflow = UiOverflow::kVisible;    ///< What happens to what overflows it.
    float grow = 0.0F;                             ///< Its share of the free space of the main axis.
    float shrink = 0.0F;                           ///< Its share of the missing space.
    UiLength basis;                                ///< Its size on the main axis before growing or shrinking.
    UiLength width;                                ///< Its width.
    UiLength height;                               ///< Its height.
    UiLength minWidth;                             ///< The narrowest it can be.
    UiLength minHeight;                            ///< The shortest it can be.
    UiLength maxWidth;                             ///< The widest it can be.
    UiLength maxHeight;                            ///< The tallest it can be.
    float aspectRatio = 0.0F;                      ///< Width over height; 0: free.
    UiLength left;                                 ///< Inset from the left edge.
    UiLength top;                                  ///< Inset from the top edge.
    UiLength right;                                ///< Inset from the right edge.
    UiLength bottom;                               ///< Inset from the bottom edge.
    UiLength marginLeft;                           ///< Space outside its left edge.
    UiLength marginTop;                            ///< Space outside its top edge.
    UiLength marginRight;                          ///< Space outside its right edge.
    UiLength marginBottom;                         ///< Space outside its bottom edge.
    UiLength paddingLeft;                          ///< Space inside its left edge.
    UiLength paddingTop;                           ///< Space inside its top edge.
    UiLength paddingRight;                         ///< Space inside its right edge.
    UiLength paddingBottom;                        ///< Space inside its bottom edge.
    float border = 0.0F;                           ///< Space its border takes on every edge, in pixels.
    float rowGap = 0.0F;                           ///< Space between its rows, in pixels.
    float columnGap = 0.0F;                        ///< Space between its columns, in pixels.
    std::int32_t zIndex = 0;                       ///< Draw order among its siblings: higher on top.
    bool ignorePointer = false;                    ///< The pointer goes through it to what is under it.
};
AYLO_ECS_COMPONENT(UiNode, "aylo.UiNode", display, position, direction, wrap, justify, alignItems, alignSelf,
                   alignContent, overflow, grow, shrink, basis, width, height, minWidth, minHeight, maxWidth, maxHeight,
                   aspectRatio, left, top, right, bottom, marginLeft, marginTop, marginRight, marginBottom, paddingLeft,
                   paddingTop, paddingRight, paddingBottom, border, rowGap, columnGap, zIndex, ignorePointer);

}  // namespace aylo::plugins::ui
