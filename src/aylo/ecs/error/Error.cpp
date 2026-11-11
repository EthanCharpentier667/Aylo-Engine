/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Error
*/

#include "Error.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace aylo::ecs {
Error::Error(ErrorKind kind, std::string message) : _kind{kind}, _message{std::move(message)} {}

ErrorKind Error::getKind() const noexcept { return _kind; }

std::string_view Error::getMessage() const noexcept { return _message; }
}  // namespace aylo::ecs
