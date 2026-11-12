/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the aylo-network library.
///
/// @details The library is static, linked into the executable: `AYLO_NETWORK_STATIC` is defined publicly, so the
/// macro expands to nothing. Built shared instead, `AYLO_NETWORK_BUILD` is defined while building it, so the symbols
/// are exported (`__declspec(dllexport)` on Windows, default visibility elsewhere) and imported by its users. Mark
/// every exported class and function with it, and keep no global mutable state, so the library works in both modes.
#ifdef AYLO_NETWORK_STATIC
#define AYLO_NETWORK_API
#else
#ifdef _WIN32
#ifdef AYLO_NETWORK_BUILD
#define AYLO_NETWORK_API __declspec(dllexport)
#else
#define AYLO_NETWORK_API __declspec(dllimport)
#endif
#else
#define AYLO_NETWORK_API __attribute__((visibility("default")))
#endif
#endif
