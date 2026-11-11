/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginException
*/

#include "PluginException.hpp"

#include <stdexcept>
#include <string>

namespace aylo::ecs::exceptions {
PluginException::PluginException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
PluginException::~PluginException() = default;
}  // namespace aylo::ecs::exceptions
