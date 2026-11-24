/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GlslShim
*/

#pragma once

#include <cmath>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_int4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/ext/vector_uint3.hpp>
#include <glm/ext/vector_uint4.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

/// @brief Enough of GLSL, over glm, for C++ to compile the engine's shader libraries (plugins/shader/builtin/*.glsl)
/// and a test to run them: their types, their functions, and what GLSL allows that glm does not, a vector and a double
/// literal (`v * 2.0`), a scalar function on mixed float and double.
/// @details The libraries are written for it: no swizzle of several components, no out parameter, no array.
namespace aylo::test::glsl {

using uint = std::uint32_t;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;
using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using glm::abs;
using glm::clamp;
using glm::cos;
using glm::cross;
using glm::dot;
using glm::floatBitsToUint;
using glm::floor;
using glm::fract;
using glm::length;
using glm::max;
using glm::min;
using glm::mix;
using glm::normalize;
using glm::pow;
using glm::sin;
using glm::sqrt;

// Scalars: float, whatever mix of float and double the call has.
inline float abs(float x) { return std::abs(x); }
inline float fract(float x) { return x - std::floor(x); }
inline float min(float x, float y) { return x < y ? x : y; }
inline float max(float x, float y) { return x > y ? x : y; }
inline float clamp(float x, float low, float high) { return min(max(x, low), high); }
inline float mix(float x, float y, float a) { return x + ((y - x) * a); }

// A vector and a scalar, the scalar a double literal or a float.
#define AYLO_GLSL_VECTOR_SCALAR(V)                                                        \
    inline V operator+(V v, float s) { return v + V(s); }                                 \
    inline V operator-(V v, float s) { return v - V(s); }                                 \
    inline V operator*(V v, float s) { return v * V(s); }                                 \
    inline V operator/(V v, float s) { return v / V(s); }                                 \
    inline V operator+(float s, V v) { return V(s) + v; }                                 \
    inline V operator-(float s, V v) { return V(s) - v; }                                 \
    inline V operator*(float s, V v) { return V(s) * v; }                                 \
    inline V operator/(float s, V v) { return V(s) / v; }                                 \
    inline V clamp(V v, float low, float high) { return glm::clamp(v, V(low), V(high)); } \
    inline V mix(V x, V y, float a) { return glm::mix(x, y, V(a)); }
AYLO_GLSL_VECTOR_SCALAR(vec2)
AYLO_GLSL_VECTOR_SCALAR(vec3)
AYLO_GLSL_VECTOR_SCALAR(vec4)
#undef AYLO_GLSL_VECTOR_SCALAR

}  // namespace aylo::test::glsl
