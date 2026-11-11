/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LibraryEntry
*/

#pragma once

#ifndef AYLO_LIBRARY_ENTRY_INCLUDING_TPP
#error "LibraryEntry.tpp is an implementation detail of LibraryEntry.hpp: include LibraryEntry.hpp instead"
#endif

#include <string>
#include <string_view>

namespace aylo::core::library {

template <typename T>
void LibraryRegistrar::provide(const std::string_view name, T& service) {
    _services.insert_or_assign(std::string{name}, static_cast<void*>(&service));
}

template <typename T>
T* LibraryRegistrar::find(const std::string_view name) const {
    const auto found = _services.find(name);
    return found != _services.end() ? static_cast<T*>(found->second) : nullptr;
}

}  // namespace aylo::core::library
