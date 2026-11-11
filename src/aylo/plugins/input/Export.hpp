/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the aylo-plugin-input library: `AYLO_PLUGIN_INPUT_BUILD` is
/// defined while building it, so the symbols are exported (`__declspec(dllexport)` on Windows, default visibility
/// elsewhere) and imported by its users. A type with no code of its own, as an interface or an exception, takes
/// AYLO_HEADER_ONLY_API.
#ifdef _WIN32
#ifdef AYLO_PLUGIN_INPUT_BUILD
#define AYLO_PLUGIN_INPUT_API __declspec(dllexport)
#else
#define AYLO_PLUGIN_INPUT_API __declspec(dllimport)
#endif
#else
#define AYLO_PLUGIN_INPUT_API __attribute__((visibility("default")))
#endif
