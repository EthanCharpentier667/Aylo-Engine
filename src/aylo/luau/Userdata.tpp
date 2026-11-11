/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Userdata
*/

#pragma once

#ifndef AYLO_USERDATA_INCLUDING_TPP
#error "Userdata.tpp is an implementation detail of Userdata.hpp: include Userdata.hpp instead"
#endif

#include <typeindex>

namespace aylo::luau {
template <typename T>
T* Userdata::tryAs() const {
    return static_cast<T*>(pointerOf(typeid(T)));
}
}  // namespace aylo::luau
