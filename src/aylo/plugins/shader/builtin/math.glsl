// @aylo/math.glsl: constants, remapping, rotations, and random numbers from a seed.
// The engine's libraries are written in a part of GLSL that C++ also reads (tests/plugins/GlslShim.hpp runs them): no
// swizzle of several components, no out parameter, no array, no preprocessor line.

const float PI = 3.14159265358979;
const float TAU = 6.28318530717959;

// value from [fromLow, fromHigh] to [toLow, toHigh], not clamped.
float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    return toLow + ((value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow));
}
vec2 remap(vec2 value, vec2 fromLow, vec2 fromHigh, vec2 toLow, vec2 toHigh) {
    return toLow + ((value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow));
}
vec3 remap(vec3 value, vec3 fromLow, vec3 fromHigh, vec3 toLow, vec3 toHigh) {
    return toLow + ((value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow));
}
vec4 remap(vec4 value, vec4 fromLow, vec4 fromHigh, vec4 toLow, vec4 toHigh) {
    return toLow + ((value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow));
}

// remap(), the result kept between toLow and toHigh.
float remapClamped(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    return clamp(remap(value, fromLow, fromHigh, toLow, toHigh), min(toLow, toHigh), max(toLow, toHigh));
}

// point turned by angle radians around the origin, counter-clockwise with Y up.
vec2 rotate(vec2 point, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec2((c * point.x) - (s * point.y), (s * point.x) + (c * point.y));
}

// point turned by angle radians around axis (any length), counter-clockwise when the axis points at you.
vec3 rotate(vec3 point, vec3 axis, float angle) {
    vec3 k = normalize(axis);
    float c = cos(angle);
    float s = sin(angle);
    return (point * c) + (cross(k, point) * s) + (k * (dot(k, point) * (1.0 - c)));
}

// A well-mixed 32-bit number from seed (PCG, O'Neill 2014, as Jarzynski and Olano 2020 use it for GPUs).
uint randomBits(uint seed) {
    uint state = (seed * 747796405u) + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// A number in [0, 1) from seed: the same seed, the same number, on every GPU.
float random(uint seed) {
    return float(randomBits(seed) >> 8u) / 16777216.0;
}

// The index-th number of the sequence seed starts: random(seed, 0u), random(seed, 1u)...
float random(uint seed, uint index) {
    return random(seed ^ randomBits(index + 1u));
}

// A number in [0, 1) from a float seed (a time, a position's coordinate).
float random(float seed) {
    return random(floatBitsToUint(seed));
}
