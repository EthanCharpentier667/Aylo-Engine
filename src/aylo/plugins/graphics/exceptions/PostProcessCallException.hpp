/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostProcessCallException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace aylo::plugins::graphics::exceptions {
/// @brief Exception thrown when `aylo.backend.setPostProcess` gets an argument it cannot use, such as an unknown
/// tonemap.
class AYLO_HEADER_ONLY_API PostProcessCallException : public std::runtime_error {
  public:
    explicit PostProcessCallException(const std::string& message) : std::runtime_error{message} {}
    ~PostProcessCallException() override = default;
    PostProcessCallException(const PostProcessCallException& other) = default;
    PostProcessCallException& operator=(const PostProcessCallException& other) = default;
    PostProcessCallException(PostProcessCallException&& other) noexcept = default;
    PostProcessCallException& operator=(PostProcessCallException&& other) noexcept = default;
};
}  // namespace aylo::plugins::graphics::exceptions
