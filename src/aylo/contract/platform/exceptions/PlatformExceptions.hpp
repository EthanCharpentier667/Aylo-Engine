/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PlatformExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace aylo::contract::platform::exceptions {
/// @brief Exception thrown when a backend is asked for an optional feature it does not implement.
///
/// This exception is thrown for example when a renderer that creates its own surface (Vulkan) is paired with a
/// platform that cannot create one: the pairing is wrong, not the call.
class AYLO_HEADER_ONLY_API UnsupportedFeatureException : public std::runtime_error {
  public:
    explicit UnsupportedFeatureException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace aylo::contract::platform::exceptions
