/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceExceptions
*/

#pragma once

#include "aylo/ecs/exceptions/TypeExceptions.hpp"

namespace aylo::ecs::exceptions {
/// @brief Exception thrown when a C++ resource cannot be registered: its name is already registered with another
/// layout or other options (presence, replication, main thread), or its description is invalid. The resource name
/// of TypeRegistrationException, so callers can catch either.
using ResourceRegistrationException = TypeRegistrationException;
}  // namespace aylo::ecs::exceptions
