/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateException
*/

#include "StateException.hpp"

#include <stdexcept>
#include <string>

namespace aylo::ecs::exceptions {
StateException::StateException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
StateException::~StateException() = default;
}  // namespace aylo::ecs::exceptions
