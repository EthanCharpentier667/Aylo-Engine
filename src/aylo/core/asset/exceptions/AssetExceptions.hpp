/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AssetExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace aylo::core::asset::exceptions {
/// @brief Exception thrown when an asset cannot be loaded: its file cannot be read, or its content cannot be decoded.
class AYLO_CORE_API AssetLoadException : public std::runtime_error {
  public:
    explicit AssetLoadException(const std::string& message) : std::runtime_error{message} {}
    ~AssetLoadException() override = default;
    AssetLoadException(const AssetLoadException& other) = default;
    AssetLoadException& operator=(const AssetLoadException& other) = default;
    AssetLoadException(AssetLoadException&& other) noexcept = default;
    AssetLoadException& operator=(AssetLoadException&& other) noexcept = default;
};
}  // namespace aylo::core::asset::exceptions
