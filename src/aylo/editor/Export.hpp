/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions of the aylo-editor library. The library is static, linked into the program:
/// `AYLO_EDITOR_STATIC` is defined publicly, so the macro expands to nothing. A type with no code of its own, as an
/// interface or an exception, takes AYLO_HEADER_ONLY_API.
#ifdef AYLO_EDITOR_STATIC
#define AYLO_EDITOR_API
#elifdef _WIN32
#ifdef AYLO_EDITOR_BUILD
#define AYLO_EDITOR_API __declspec(dllexport)
#else
#define AYLO_EDITOR_API __declspec(dllimport)
#endif
#else
#define AYLO_EDITOR_API __attribute__((visibility("default")))
#endif
