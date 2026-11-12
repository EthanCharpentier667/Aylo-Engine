/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GlfwMapping
*/

#pragma once

#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/platform/glfw/Export.hpp"

/// @brief Translation between GLFW codes and the engine input types.
///
/// @details This is the only place that knows both. GLFW codes never leave the
/// platform layer: everything is translated into engine types before being sent
/// as an event.
namespace aylo::platform::glfw {

/// @return The engine key for a GLFW key code, or Key::kUnknown.
[[nodiscard]] AYLO_PLATFORM_GLFW_API contract::platform::input::Key fromGlfwKey(int key);
/// @return The GLFW key code for an engine key, or GLFW_KEY_UNKNOWN.
[[nodiscard]] AYLO_PLATFORM_GLFW_API int toGlfwKey(contract::platform::input::Key key);

/// @return The engine mouse button for a GLFW button code, or MouseButton::kUnknown.
[[nodiscard]] AYLO_PLATFORM_GLFW_API contract::platform::input::MouseButton fromGlfwMouseButton(int button) noexcept;
/// @return The GLFW button code for an engine mouse button, or -1 for MouseButton::kUnknown.
[[nodiscard]] AYLO_PLATFORM_GLFW_API int toGlfwMouseButton(contract::platform::input::MouseButton button) noexcept;

/// @return The GLFW gamepad button index for an engine gamepad button.
[[nodiscard]] AYLO_PLATFORM_GLFW_API int toGlfwGamepadButton(contract::platform::input::GamepadButton button) noexcept;
/// @return The GLFW gamepad axis index for an engine gamepad axis.
[[nodiscard]] AYLO_PLATFORM_GLFW_API int toGlfwGamepadAxis(contract::platform::input::GamepadAxis axis) noexcept;

/// @return A raw GLFW axis value converted to the engine convention: stick Y up-positive, triggers in [0, 1].
[[nodiscard]] AYLO_PLATFORM_GLFW_API float fromGlfwGamepadAxisValue(contract::platform::input::GamepadAxis axis,
                                                                    float value) noexcept;

/// @return The engine modifiers for a GLFW modifier bit field.
[[nodiscard]] AYLO_PLATFORM_GLFW_API contract::platform::input::Mods fromGlfwMods(int mods) noexcept;

}  // namespace aylo::platform::glfw
