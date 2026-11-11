/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TypeExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "aylo/ecs/Export.hpp"

namespace aylo::ecs::exceptions {
/// @brief Exception thrown when a C++ type (a component, a resource) cannot be registered: its name is already
/// registered with another layout or other options, or its description is invalid.
class AYLO_ECS_API TypeRegistrationException : public std::runtime_error {
  public:
    explicit TypeRegistrationException(const std::string& message) : std::runtime_error{message} {}
    ~TypeRegistrationException() override = default;
    TypeRegistrationException(const TypeRegistrationException& other) = default;
    TypeRegistrationException& operator=(const TypeRegistrationException& other) = default;
    TypeRegistrationException(TypeRegistrationException&& other) noexcept = default;
    TypeRegistrationException& operator=(TypeRegistrationException&& other) noexcept = default;
};
}  // namespace aylo::ecs::exceptions
