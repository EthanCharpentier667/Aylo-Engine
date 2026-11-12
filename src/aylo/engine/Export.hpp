/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the aylo-engine library.
///
/// @details The library is static, linked into the executable: `AYLO_ENGINE_STATIC` is defined publicly, so the
/// macro expands to nothing. Built shared instead, `AYLO_ENGINE_BUILD` is defined while building it, so the symbols
/// are exported (`__declspec(dllexport)` on Windows, default visibility elsewhere) and imported by its users.
#ifdef AYLO_ENGINE_STATIC
#define AYLO_ENGINE_API
#else
#ifdef _WIN32
#ifdef AYLO_ENGINE_BUILD
#define AYLO_ENGINE_API __declspec(dllexport)
#else
#define AYLO_ENGINE_API __declspec(dllimport)
#endif
#else
#define AYLO_ENGINE_API __attribute__((visibility("default")))
#endif
#endif
