/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ControlNamesTests
*/

#include <gtest/gtest.h>

#include <optional>
#include <string_view>

#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/contract/platform/input/KeyNames.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/ControlNames.hpp"

using aylo::contract::platform::input::GamepadAxis;
using aylo::contract::platform::input::GamepadButton;
using aylo::contract::platform::input::Key;
using aylo::contract::platform::input::MouseButton;
using aylo::plugins::input::Control;
using aylo::plugins::input::controlFromName;

namespace {
/// @return Whether @p name names exactly @p expected.
[[nodiscard]] bool names(const std::string_view name, const Control& expected) {
    const std::optional<Control> control = controlFromName(name);
    return control && control->source == expected.source && control->code == expected.code &&
           control->scale == expected.scale;
}
}  // namespace

TEST(KeyNames, FindsKeysByTheirEnumeratorName) {
    EXPECT_EQ(aylo::contract::platform::input::keyFromName("W"), Key::kW);
    EXPECT_EQ(aylo::contract::platform::input::keyFromName("Space"), Key::kSpace);
    EXPECT_EQ(aylo::contract::platform::input::keyFromName("Num1"), Key::kNum1);
    EXPECT_EQ(aylo::contract::platform::input::keyFromName("Up"), Key::kUp);
    EXPECT_EQ(aylo::contract::platform::input::keyFromName("F12"), Key::kF12);
    EXPECT_EQ(aylo::contract::platform::input::keyFromName("KeypadEnter"), Key::kKeypadEnter);
    EXPECT_EQ(aylo::contract::platform::input::keyFromName("Menu"), Key::kMenu);
}

TEST(KeyNames, RefusesUnknownAndMiscasedNames) {
    EXPECT_FALSE(aylo::contract::platform::input::keyFromName("w"));
    EXPECT_FALSE(aylo::contract::platform::input::keyFromName("Unknown"));
    EXPECT_FALSE(aylo::contract::platform::input::keyFromName(""));
    EXPECT_FALSE(aylo::contract::platform::input::mouseButtonFromName("Unknown"));
    EXPECT_FALSE(aylo::contract::platform::input::gamepadAxisFromName("Count"));
}

TEST(KeyNames, FindsMouseAndGamepadControls) {
    EXPECT_EQ(aylo::contract::platform::input::mouseButtonFromName("Left"), MouseButton::kLeft);
    EXPECT_EQ(aylo::contract::platform::input::mouseButtonFromName("X2"), MouseButton::kX2);
    EXPECT_EQ(aylo::contract::platform::input::gamepadButtonFromName("South"), GamepadButton::kSouth);
    EXPECT_EQ(aylo::contract::platform::input::gamepadButtonFromName("DpadLeft"), GamepadButton::kDpadLeft);
    EXPECT_EQ(aylo::contract::platform::input::gamepadAxisFromName("LeftX"), GamepadAxis::kLeftX);
    EXPECT_EQ(aylo::contract::platform::input::gamepadAxisFromName("RightTrigger"), GamepadAxis::kRightTrigger);
}

TEST(ControlNames, NamesEveryKindOfControl) {
    EXPECT_TRUE(names("W", Control::key(Key::kW)));
    EXPECT_TRUE(names("Left", Control::key(Key::kLeft)));  // the arrow key: mouse buttons take "Mouse"
    EXPECT_TRUE(names("MouseLeft", Control::mouseButton(MouseButton::kLeft)));
    EXPECT_TRUE(names("MouseDeltaX", Control::mouseDeltaX()));
    EXPECT_TRUE(names("MouseDeltaY", Control::mouseDeltaY()));
    EXPECT_TRUE(names("MouseScrollX", Control::mouseScrollX()));
    EXPECT_TRUE(names("MouseScrollY", Control::mouseScrollY()));
    EXPECT_TRUE(names("GamepadSouth", Control::gamepadButton(GamepadButton::kSouth)));
    EXPECT_TRUE(names("GamepadLeftY", Control::gamepadAxis(GamepadAxis::kLeftY)));
}

TEST(ControlNames, RefusesWhatNamesNoControl) {
    EXPECT_FALSE(controlFromName("Mouse"));
    EXPECT_FALSE(controlFromName("MouseW"));
    EXPECT_FALSE(controlFromName("GamepadUp"));
    EXPECT_FALSE(controlFromName("Gamepad"));
    EXPECT_FALSE(controlFromName("Jump"));
}
