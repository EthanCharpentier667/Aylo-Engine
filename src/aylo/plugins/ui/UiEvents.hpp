/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** UiEvents
*/

#pragma once

#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/type/String.hpp"

namespace aylo::plugins::ui {

/// @brief A UiButton was activated, by the pointer, the keyboard or a gamepad.
struct UiClicked {
    ecs::Entity entity;  ///< The button.
    ecs::String action;  ///< Its UiButton::action.
};
AYLO_ECS_EVENT(UiClicked, "rtype.UiClicked", entity, action);

/// @brief The value of a UiSlider or a UiToggle (0 or 1) changed.
struct UiValueChanged {
    ecs::Entity entity;  ///< The widget.
    ecs::String action;  ///< Its action.
    float value = 0.0F;  ///< Its new value.
};
AYLO_ECS_EVENT(UiValueChanged, "rtype.UiValueChanged", entity, action, value);

/// @brief The text of a UiTextInput changed: read it from its UiText.
struct UiTextChanged {
    ecs::Entity entity;  ///< The input.
    ecs::String action;  ///< Its action.
};
AYLO_ECS_EVENT(UiTextChanged, "rtype.UiTextChanged", entity, action);

/// @brief Enter was pressed in a UiTextInput.
struct UiTextSubmitted {
    ecs::Entity entity;  ///< The input.
    ecs::String action;  ///< Its action.
};
AYLO_ECS_EVENT(UiTextSubmitted, "rtype.UiTextSubmitted", entity, action);

}  // namespace aylo::plugins::ui
