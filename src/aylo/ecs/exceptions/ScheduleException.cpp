/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScheduleException
*/

#include "ScheduleException.hpp"

#include <stdexcept>
#include <string>

namespace aylo::ecs::exceptions {
ScheduleException::ScheduleException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
ScheduleException::~ScheduleException() = default;
}  // namespace aylo::ecs::exceptions
