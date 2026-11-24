/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Environment
*/

// What the environment's passes share (rendering/EnvironmentMaps.hpp): the equirectangular mapping, from a direction
// to where it is in an image twice as wide as high, the zenith at its top, -Z at its centre.
#ifndef AYLO_ENVIRONMENT_GLSL
#define AYLO_ENVIRONMENT_GLSL

const float AYLO_ENVIRONMENT_PI = 3.14159265358979;

vec2 ayloEquirectUv(vec3 direction) {
    return vec2((atan(direction.x, -direction.z) / (2.0 * AYLO_ENVIRONMENT_PI)) + 0.5,
                acos(clamp(direction.y, -1.0, 1.0)) / AYLO_ENVIRONMENT_PI);
}

vec3 ayloEquirectDirection(vec2 uv) {
    const float phi = (uv.x - 0.5) * 2.0 * AYLO_ENVIRONMENT_PI;
    const float theta = uv.y * AYLO_ENVIRONMENT_PI;
    return vec3(sin(theta) * sin(phi), cos(theta), -sin(theta) * cos(phi));
}

#endif
