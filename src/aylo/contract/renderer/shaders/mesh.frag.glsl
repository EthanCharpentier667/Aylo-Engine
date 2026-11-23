/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** mesh.frag
*/

// The fragment stage of static meshes (`#pragma mesh`): the texture times the colour, both premultiplied, unlit, as the
// built-in static mesh shader draws them; lit (AYLO_LIT, `#pragma lit`) from the surface fragment() writes, by
// lighting.glsl's model or the shader's light() (AYLO_CUSTOM_LIGHT).
layout(location = 3) in vec3 ayloWorldIn;
layout(location = 4) in vec3 ayloLocalIn;
layout(location = 5) in vec4 ayloTangentIn;  // In the world; w: the bitangent's sign.
layout(location = 6) in vec4 ayloVertexColorIn;
layout(location = 8) in vec2 ayloUv2In;
layout(location = 7) in vec3 ayloNormalIn;  // In the world.
vec3 LOCAL_POSITION;  // The pixel, in the model's space.
vec3 VIEW_DIR;        // Unit, from the pixel to the camera (a perspective), or toward an orthographic camera.
#define FRONT_FACING gl_FrontFacing  // Whether the triangle is seen from its front (counter-clockwise) side.
#ifdef AYLO_ALPHA_SCISSOR
float ALPHA_SCISSOR;  // A pixel whose COLOR.a is below it is not drawn, the others are opaque (cutout); 0: off.
#endif

#pragma aylo_shader

#ifdef AYLO_LIT
// The direction, colour and attenuation of light `index` at `position` (LIGHT_DIR, LIGHT_COLOR, ATTENUATION), for one
// punctual light; false for an area light, lit apart.
bool ayloSetLight(uint index, vec3 position) {
    const AyloLight light = ayloLights.light[index];
    const uint kind = uint(light.direction.w + 0.5);
    LIGHT_COLOR = light.color.rgb * light.color.w;
    if (kind == 0u) {  // directional: lux
        LIGHT_DIR = -light.direction.xyz;
        ATTENUATION = 1.0;
        return true;
    }
    if (kind == 3u) {
        return false;
    }
    const vec3 toLight = light.position.xyz - position;
    const float d = length(toLight);
    LIGHT_DIR = toLight / max(d, 1e-5);
    LIGHT_COLOR /= 4.0 * AYLO_PI;  // watts to W/sr
    ATTENUATION = ayloDistanceFalloff(d, light.position.w);
    if (kind == 2u) {  // spot: the cone, smoothly
        ATTENUATION *= smoothstep(light.cone.y, light.cone.x, dot(-LIGHT_DIR, light.direction.xyz));
    }
    return true;
}

// What area light `index` gives the surface (n, v at position), added to DIFFUSE_LIGHT and SPECULAR_LIGHT: the
// rectangle's exact irradiance for the diffuse half, and its representative point (the point of the rectangle nearest
// the reflected ray, Karis 2013) with a widened lobe for the specular half.
void ayloAreaLight(uint index, vec3 position, vec3 n, vec3 v) {
    const AyloLight light = ayloLights.light[index];
    const vec3 facing = light.direction.xyz;
    if (dot(position - light.position.xyz, facing) <= 0.0) {
        return;  // behind it
    }
    const vec3 right = light.right.xyz * light.cone.z;
    const vec3 up = normalize(cross(light.right.xyz, facing)) * light.cone.w;
    const vec3 center = light.position.xyz - position;
    vec3 corners[4];
    corners[0] = center - right - up;
    corners[1] = center + right - up;
    corners[2] = center + right + up;
    corners[3] = center - right + up;
    const float area = 4.0 * light.cone.z * light.cone.w;
    const vec3 radiance = light.color.rgb * light.color.w / (AYLO_PI * max(area, 1e-4));
    const float rangeFalloff = light.position.w > 0.0
                                   ? clamp(1.0 - pow(length(center) / light.position.w, 4.0), 0.0, 1.0)
                                   : 1.0;
    const float irradiance = ayloRectangleIrradiance(n, corners) * rangeFalloff;
    const vec3 f0 = mix(vec3(0.08 * SPECULAR), ALBEDO, METALLIC);
    const float nov = clamp(abs(dot(n, v)), 1e-4, 1.0);
    DIFFUSE_LIGHT += (1.0 - METALLIC) * ALBEDO / AYLO_PI * radiance * irradiance * SHADOW;
    // The representative point: where the reflected ray meets the light's plane, kept inside the rectangle.
    const vec3 reflected = reflect(-v, n);
    const float along = dot(reflected, facing);
    vec3 hit = center;
    if (along < -1e-4) {
        hit = reflected * (dot(center, facing) / along);
    }
    const vec3 local = hit - center;
    const vec3 clamped = center + (light.right.xyz * clamp(dot(local, light.right.xyz), -light.cone.z, light.cone.z)) +
                         (normalize(up) * clamp(dot(local, normalize(up)), -light.cone.w, light.cone.w));
    const float d = max(length(clamped), 1e-4);
    const vec3 l = clamped / d;
    const float nol = clamp(dot(n, l), 0.0, 1.0);
    const float alpha = max(ROUGHNESS * ROUGHNESS, 2e-3);
    // The lobe widened by the light's size seen from here, its energy kept (Karis's sphere normalization).
    const float widened = clamp(alpha + (0.5 * sqrt(area) / d), alpha, 1.0);
    const float normalization = (alpha / widened) * (alpha / widened);
    const vec3 h = normalize(l + v);
    const float solidAngle = min(area * clamp(dot(-l, facing), 0.0, 1.0) / (d * d), 2.0 * AYLO_PI);
    SPECULAR_LIGHT += ayloFresnel(f0, dot(v, h)) * ayloDistribution(clamp(dot(n, h), 0.0, 1.0), widened) *
                      ayloVisibility(nol, nov, widened) * nol * normalization * radiance *
                      max(solidAngle, area / (d * d + area)) * rangeFalloff * SHADOW;
}

// Sets LIGHT_DIR, LIGHT_COLOR and ATTENUATION for area light `index` seen as one light, for a shader's light(): toward
// its centre, the colour giving the rectangle's irradiance. False when the point is behind it.
bool ayloAreaAsPoint(uint index, vec3 position, vec3 n) {
    const AyloLight light = ayloLights.light[index];
    const vec3 facing = light.direction.xyz;
    if (dot(position - light.position.xyz, facing) <= 0.0) {
        return false;
    }
    const vec3 right = light.right.xyz * light.cone.z;
    const vec3 up = normalize(cross(light.right.xyz, facing)) * light.cone.w;
    const vec3 center = light.position.xyz - position;
    vec3 corners[4];
    corners[0] = center - right - up;
    corners[1] = center + right - up;
    corners[2] = center + right + up;
    corners[3] = center - right + up;
    const float area = 4.0 * light.cone.z * light.cone.w;
    LIGHT_DIR = normalize(center);
    const float nol = max(dot(n, LIGHT_DIR), 1e-3);
    LIGHT_COLOR = light.color.rgb * light.color.w / (AYLO_PI * max(area, 1e-4)) *
                  ayloRectangleIrradiance(n, corners) / nol;
    ATTENUATION = light.position.w > 0.0 ? clamp(1.0 - pow(length(center) / light.position.w, 4.0), 0.0, 1.0) : 1.0;
    return true;
}

// The lit pixel, premultiplied: every light (by light() if the shader has one, else by the PBR model), the ambient
// light and the emission.
vec4 ayloLightSurface(vec3 position) {
    const vec3 n = NORMAL;
    const vec3 v = VIEW_DIR;
    DIFFUSE_LIGHT = vec3(0.0);
    SPECULAR_LIGHT = vec3(0.0);
    for (uint index = 0u; index < ayloLights.count.x; ++index) {
        SHADOW = ayloShadowOf(index, position, n);
        if (!ayloSetLight(index, position)) {
#ifdef AYLO_CUSTOM_LIGHT
            // light() sees an area light as one coming from its centre, LIGHT_COLOR such that LIGHT_COLOR * n.l is the
            // irradiance the whole rectangle gives, as for a directional light.
            if (ayloAreaAsPoint(index, position, n)) {
                light();
            }
#else
            ayloAreaLight(index, position, n, v);
#endif
            continue;
        }
#ifdef AYLO_CUSTOM_LIGHT
        light();
#else
        vec3 diffuse;
        vec3 reflected;
        ayloBrdf(n, v, LIGHT_DIR, ALBEDO, METALLIC, ROUGHNESS, SPECULAR, diffuse, reflected);
        const vec3 received = LIGHT_COLOR * ATTENUATION * SHADOW;
        DIFFUSE_LIGHT += diffuse * received;
        SPECULAR_LIGHT += reflected * received;
#endif
    }
    const vec3 f0 = mix(vec3(0.08 * SPECULAR), ALBEDO, METALLIC);
    const float nov = clamp(abs(dot(n, v)), 1e-4, 1.0);
    const vec3 ambient = ayloLights.ambient.rgb * AO *
                         (((1.0 - METALLIC) * ALBEDO) + ayloEnvironmentBrdf(f0, ROUGHNESS, nov));
    const vec3 rgb = DIFFUSE_LIGHT + SPECULAR_LIGHT + ambient + EMISSION;
    return vec4(rgb * ALPHA, ALPHA);
}
#endif

void main() {
    ayloLoadMeshDraw();
    WORLD_POSITION = ayloWorldIn;
    LOCAL_POSITION = ayloLocalIn;
    VIEW_DIR = ayloFrame.cameraPosition.w > 0.5 ? normalize(ayloFrame.cameraPosition.xyz - ayloWorldIn)
                                                : normalize(ayloFrame.inverseView[2].xyz);
    UV = ayloUvIn;
    UV2 = ayloUv2In;
    VERTEX_COLOR = ayloVertexColorIn;
    NORMAL = length(ayloNormalIn) > 0.0 ? normalize(ayloNormalIn) : ayloNormalIn;
    TANGENT = length(ayloTangentIn.xyz) > 0.0 ? normalize(ayloTangentIn.xyz) : vec3(0.0);
    BINORMAL = cross(NORMAL, TANGENT) * ayloTangentIn.w;
    COLOR = texture(TEXTURE, UV) * ayloColorIn;
#ifdef AYLO_LIT
    // The surface starts as the part's colours and texture; a face seen from behind is lit from its side.
    if (!gl_FrontFacing) {
        NORMAL = -NORMAL;
    }
    ALBEDO = COLOR.a > 0.0 ? COLOR.rgb / COLOR.a : vec3(0.0);
    ALPHA = COLOR.a;
    METALLIC = ayloPart.surface.x;
    ROUGHNESS = ayloPart.surface.y;
    if ((ayloPart.flags & ayloPartHasMetallicRoughness) != 0u) {
        const vec4 ayloMr = texture(ayloPartMetallicRoughness, UV);
        METALLIC *= ayloMr.b;
        ROUGHNESS *= ayloMr.g;
    }
    SPECULAR = 0.5;
    EMISSION = ayloPart.emissive.rgb;
    if ((ayloPart.flags & ayloPartHasEmissive) != 0u) {
        EMISSION *= texture(ayloPartEmissive, UV).rgb;
    }
    AO = 1.0;
    if ((ayloPart.flags & ayloPartHasOcclusion) != 0u) {
        AO = mix(1.0, texture(ayloPartOcclusion, UV).r, ayloPart.surface.z);
    }
    if ((ayloPart.flags & ayloPartHasNormal) != 0u && dot(TANGENT, TANGENT) > 0.0) {
        // A tangent-space normal map, its X and Y scaled, turned to the world by the tangent frame.
        vec3 ayloMapped = (texture(ayloPartNormal, UV).xyz * 2.0) - 1.0;
        ayloMapped.xy *= ayloPart.emissive.w;
        NORMAL = normalize(mat3(TANGENT, BINORMAL, NORMAL) * ayloMapped);
    }
#endif
#ifdef AYLO_ALPHA_SCISSOR
    ALPHA_SCISSOR = 0.0;
#endif
#pragma aylo_main
#ifdef AYLO_BUILTIN_MESH
    // The built-in material follows the part's alpha mode: opaque ignores the alpha, a mask cuts it out.
    const uint ayloAlphaMode = (ayloPart.flags >> 4u) & 3u;
    if (ayloAlphaMode == 1u && COLOR.a < ayloPart.surface.w) {
        discard;
    }
#ifdef AYLO_LIT
    if (ayloAlphaMode != 2u) {
        ALPHA = 1.0;
    }
#else
    if (ayloAlphaMode != 2u && COLOR.a > 0.0) {
        COLOR = vec4(COLOR.rgb / COLOR.a, 1.0);
    }
#endif
#endif
#ifdef AYLO_LIT
    NORMAL = length(NORMAL) > 0.0 ? normalize(NORMAL) : NORMAL;
    COLOR = ayloLightSurface(WORLD_POSITION);
#endif
#ifdef AYLO_ALPHA_SCISSOR
    if (ALPHA_SCISSOR > 0.0) {
        if (COLOR.a < ALPHA_SCISSOR) {
            discard;
        }
        COLOR = vec4(COLOR.rgb / COLOR.a, 1.0);
    }
#endif
    ayloFragColor = COLOR;
}
