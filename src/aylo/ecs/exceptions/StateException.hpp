/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "aylo/ecs/Export.hpp"

namespace aylo::ecs::exceptions {
/// @brief Exception thrown when a state machine is misused: declared twice with other values, or asked for a machine or
/// a value it does not have. The message names the machine and the value.
/// @details Exported, with its destructor defined in the library, as the other exceptions of the ECS.
class AYLO_ECS_API StateException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the machine and the value.
    explicit StateException(const std::string& message);
    ~StateException() override;
    StateException(const StateException& other) = default;
    StateException& operator=(const StateException& other) = default;
    StateException(StateException&& other) noexcept = default;
    StateException& operator=(StateException&& other) noexcept = default;
};
}  // namespace aylo::ecs::exceptions
