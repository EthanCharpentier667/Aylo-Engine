/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the aylo-luau library.
///
/// @details The library is static, linked into the executable: `AYLO_LUAU_STATIC` is defined publicly, so the macro
/// expands to nothing (a `__declspec(dllimport)` would ask for symbols no DLL defines). Switching the target to a
/// shared library means dropping that define: `AYLO_LUAU_BUILD` then exports the symbols (`__declspec(dllexport)` on
/// Windows, default visibility elsewhere) and its users import them.
#ifdef AYLO_LUAU_STATIC
#define AYLO_LUAU_API
#else
#ifdef _WIN32
#ifdef AYLO_LUAU_BUILD
#define AYLO_LUAU_API __declspec(dllexport)
#else
#define AYLO_LUAU_API __declspec(dllimport)
#endif
#else
#define AYLO_LUAU_API __attribute__((visibility("default")))
#endif
#endif
