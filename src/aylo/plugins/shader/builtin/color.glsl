// @aylo/color.glsl: colour spaces, luminance and blend modes.
// Written in the part of GLSL that C++ also reads (see @aylo/math.glsl).

// One channel from sRGB, as an image stores it, to linear, as light adds up.
float srgbToLinear(float channel) {
    return channel <= 0.04045 ? channel / 12.92 : pow((channel + 0.055) / 1.055, 2.4);
}
vec3 srgbToLinear(vec3 color) {
    return vec3(srgbToLinear(color.x), srgbToLinear(color.y), srgbToLinear(color.z));
}
vec4 srgbToLinear(vec4 color) {
    return vec4(srgbToLinear(color.x), srgbToLinear(color.y), srgbToLinear(color.z), color.w);
}

// One channel from linear to sRGB.
float linearToSrgb(float channel) {
    return channel <= 0.0031308 ? channel * 12.92 : (1.055 * pow(channel, 1.0 / 2.4)) - 0.055;
}
vec3 linearToSrgb(vec3 color) {
    return vec3(linearToSrgb(color.x), linearToSrgb(color.y), linearToSrgb(color.z));
}
vec4 linearToSrgb(vec4 color) {
    return vec4(linearToSrgb(color.x), linearToSrgb(color.y), linearToSrgb(color.z), color.w);
}

// How bright a linear colour looks (Rec. 709 weights).
float luminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// Hue (0 to 1, red at 0), saturation and value of an RGB colour.
vec3 rgbToHsv(vec3 color) {
    float high = max(color.x, max(color.y, color.z));
    float low = min(color.x, min(color.y, color.z));
    float delta = high - low;
    float hue = 0.0;
    if (delta > 0.0) {
        if (high == color.x) {
            hue = (color.y - color.z) / delta;
        } else if (high == color.y) {
            hue = 2.0 + ((color.z - color.x) / delta);
        } else {
            hue = 4.0 + ((color.x - color.y) / delta);
        }
        hue = fract(hue / 6.0);
    }
    float saturation = high > 0.0 ? delta / high : 0.0;
    return vec3(hue, saturation, high);
}

// The RGB colour of a hue (0 to 1, wrapping), saturation and value.
vec3 hsvToRgb(vec3 hsv) {
    vec3 channels = clamp(abs((fract(vec3(hsv.x) + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return hsv.z * mix(vec3(1.0), channels, hsv.y);
}

// The blend modes: blend laid over base, then mixed with base by opacity (0: base, 1: the blend mode's result).
vec3 blendMultiply(vec3 base, vec3 blend, float opacity) {
    return mix(base, base * blend, opacity);
}
vec3 blendScreen(vec3 base, vec3 blend, float opacity) {
    return mix(base, vec3(1.0) - ((vec3(1.0) - base) * (vec3(1.0) - blend)), opacity);
}
float blendOverlay(float base, float blend) {
    return base < 0.5 ? 2.0 * base * blend : 1.0 - (2.0 * (1.0 - base) * (1.0 - blend));
}
vec3 blendOverlay(vec3 base, vec3 blend, float opacity) {
    vec3 result = vec3(blendOverlay(base.x, blend.x), blendOverlay(base.y, blend.y), blendOverlay(base.z, blend.z));
    return mix(base, result, opacity);
}
// The W3C compositing formula.
float blendSoftLight(float base, float blend) {
    if (blend <= 0.5) {
        return base - ((1.0 - (2.0 * blend)) * base * (1.0 - base));
    }
    float lifted = base <= 0.25 ? ((((16.0 * base) - 12.0) * base) + 4.0) * base : sqrt(base);
    return base + (((2.0 * blend) - 1.0) * (lifted - base));
}
vec3 blendSoftLight(vec3 base, vec3 blend, float opacity) {
    vec3 result =
        vec3(blendSoftLight(base.x, blend.x), blendSoftLight(base.y, blend.y), blendSoftLight(base.z, blend.z));
    return mix(base, result, opacity);
}
vec3 blendLinearLight(vec3 base, vec3 blend, float opacity) {
    return mix(base, base + (2.0 * blend) - 1.0, opacity);
}
vec3 blendAdd(vec3 base, vec3 blend, float opacity) {
    return mix(base, base + blend, opacity);
}
vec3 blendSubtract(vec3 base, vec3 blend, float opacity) {
    return mix(base, max(base - blend, vec3(0.0)), opacity);
}
vec3 blendDifference(vec3 base, vec3 blend, float opacity) {
    return mix(base, abs(base - blend), opacity);
}
vec3 blendDarken(vec3 base, vec3 blend, float opacity) {
    return mix(base, min(base, blend), opacity);
}
vec3 blendLighten(vec3 base, vec3 blend, float opacity) {
    return mix(base, max(base, blend), opacity);
}
float blendColorDodge(float base, float blend) {
    return blend >= 1.0 ? 1.0 : min(1.0, base / (1.0 - blend));
}
vec3 blendColorDodge(vec3 base, vec3 blend, float opacity) {
    vec3 result =
        vec3(blendColorDodge(base.x, blend.x), blendColorDodge(base.y, blend.y), blendColorDodge(base.z, blend.z));
    return mix(base, result, opacity);
}
float blendColorBurn(float base, float blend) {
    return blend <= 0.0 ? 0.0 : 1.0 - min(1.0, (1.0 - base) / blend);
}
vec3 blendColorBurn(vec3 base, vec3 blend, float opacity) {
    vec3 result =
        vec3(blendColorBurn(base.x, blend.x), blendColorBurn(base.y, blend.y), blendColorBurn(base.z, blend.z));
    return mix(base, result, opacity);
}
