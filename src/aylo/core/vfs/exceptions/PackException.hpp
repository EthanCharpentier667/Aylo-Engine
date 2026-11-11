/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PackException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace aylo::core::vfs::exceptions {
/// @brief A pack that cannot be read (missing, damaged, not a pack) or written; the message names the file and why.
class AYLO_CORE_API PackException : public std::runtime_error {
  public:
    explicit PackException(const std::string& message) : std::runtime_error{message} {}
    ~PackException() override = default;
    PackException(const PackException& other) = default;
    PackException& operator=(const PackException& other) = default;
    PackException(PackException&& other) noexcept = default;
    PackException& operator=(PackException&& other) noexcept = default;
};
}  // namespace aylo::core::vfs::exceptions
