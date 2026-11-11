/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BackendExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace aylo::contract::platform::exceptions {
/// @brief Exception thrown when the configuration names a platform or a renderer that was not registered.
class AYLO_HEADER_ONLY_API BackendException : public std::runtime_error {
  public:
    explicit BackendException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace aylo::contract::platform::exceptions
