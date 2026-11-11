/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the aylo-plugin-persistence library:
/// `AYLO_PLUGIN_PERSISTENCE_BUILD` is defined while building it.
#ifdef _WIN32
#ifdef AYLO_PLUGIN_PERSISTENCE_BUILD
#define AYLO_PLUGIN_PERSISTENCE_API __declspec(dllexport)
#else
#define AYLO_PLUGIN_PERSISTENCE_API __declspec(dllimport)
#endif
#else
#define AYLO_PLUGIN_PERSISTENCE_API __attribute__((visibility("default")))
#endif
