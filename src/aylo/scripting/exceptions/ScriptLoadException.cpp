/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptLoadException
*/

#include "aylo/scripting/exceptions/ScriptLoadException.hpp"

#include <stdexcept>
#include <string>

namespace aylo::scripting::exceptions {
ScriptLoadException::ScriptLoadException(const std::string& message) : std::runtime_error{message} {}

ScriptLoadException::~ScriptLoadException() = default;
}  // namespace aylo::scripting::exceptions
