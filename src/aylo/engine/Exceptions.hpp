/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Exceptions
*/

#pragma once

#include <stdexcept>

#include "Export.hpp"

namespace aylo::engine::exceptions {
/// @brief Exception thrown when the engine fails while running, for instance when a script cannot be loaded.
///
/// It can be caught as a `std::runtime_error`.
class AYLO_ENGINE_API RuntimeError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};
}  // namespace aylo::engine::exceptions
