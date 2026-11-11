/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

/// @brief The macro that exports symbols from the Vulkan module, for the shared library build.
/// @details The Vulkan module is a shared library, so its symbols must be exported for the engine to use them.
/// Without this, the engine will not find the VulkanRenderer class, and the registration function will not be called.
/// Dynamic cast to IVulkanSurfaceSource will fail, and the engine will throw UnsupportedFeatureException.
#ifdef _WIN32
#ifdef AYLO_RENDER_VULKAN_BUILD
#define AYLO_RENDER_VULKAN_API __declspec(dllexport)
#else
#define AYLO_RENDER_VULKAN_API __declspec(dllimport)
#endif
#else
#define AYLO_RENDER_VULKAN_API __attribute__((visibility("default")))
#endif
