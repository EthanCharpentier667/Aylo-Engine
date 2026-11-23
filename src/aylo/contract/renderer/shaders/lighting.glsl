/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** lighting
*/

// The lighting of `#pragma lit` 3D shaders (AYLO_LIT): the metallic-roughness model of glTF 2.0 (appendix B): GGX
// distribution, height-correlated Smith visibility, Schlick Fresnel, Lambert diffuse, and what each kind of light gives
// a surface. Physical units, as the lights' (watts, lux): a point light of power P has an intensity of P / (4 pi) W/sr,
// an area light of power P and area A a radiance of P / (pi A), a directional one lights with its intensity in lux. The
// result is linear, in the half-float scene: tonemapping is a screen effect.

const float AYLO_PI = 3.14159265358979;

// The surface a lit shader writes in fragment(), lit after it. Its defaults come from the part's colours and texture.
vec3 ALBEDO;      // Base colour, linear, not premultiplied.
float ALPHA;      // Opacity (blended materials).
float METALLIC;   // 0: dielectric, 1: metal.
float ROUGHNESS;  // 0: mirror, 1: rough.
float SPECULAR;   // A dielectric's reflectance: 0.5 is 4%, the glTF default.
vec3 EMISSION;    // Light it gives off, added as it is (linear, may exceed 1).
float AO;         // Ambient occlusion: how much of the ambient light reaches it.

// One light, for a shader's light(): it adds what that light gives to DIFFUSE_LIGHT and SPECULAR_LIGHT.
vec3 LIGHT_DIR;       // Unit, from the surface toward the light.
vec3 LIGHT_COLOR;     // Its colour times its strength at the surface, before ATTENUATION: W/m^2 facing it.
float ATTENUATION;    // How much of it reaches this point: distance, range and cone.
float SHADOW;         // 1: lit, 0: in its shadow.
vec3 DIFFUSE_LIGHT;   // What every light gave by diffusion so far (the default light() multiplies by the albedo).
vec3 SPECULAR_LIGHT;  // What every light gave by reflection so far.

// GGX normal distribution, alpha = roughness squared.
float ayloDistribution(float noh, float alpha) {
    const float alpha2 = alpha * alpha;
    const float d = (noh * noh * (alpha2 - 1.0)) + 1.0;
    return alpha2 / (AYLO_PI * d * d);
}

// Height-correlated Smith visibility, the BRDF's G / (4 n.l n.v).
float ayloVisibility(float nol, float nov, float alpha) {
    const float alpha2 = alpha * alpha;
    const float view = nol * sqrt((nov * nov * (1.0 - alpha2)) + alpha2);
    const float light = nov * sqrt((nol * nol * (1.0 - alpha2)) + alpha2);
    return 0.5 / max(view + light, 1e-5);
}

// Schlick's Fresnel.
vec3 ayloFresnel(vec3 f0, float voh) {
    return f0 + ((1.0 - f0) * pow(1.0 - clamp(voh, 0.0, 1.0), 5.0));
}

// What the surface reflects of light coming from l, per unit of irradiance facing it, times n.l: the PBR shading of one
// light. diffuse and specular are its two halves.
void ayloBrdf(vec3 n, vec3 v, vec3 l, vec3 albedo, float metallic, float roughness, float specular,
              out vec3 diffuse, out vec3 reflected) {
    const vec3 h = normalize(l + v);
    const float nol = clamp(dot(n, l), 0.0, 1.0);
    const float nov = clamp(abs(dot(n, v)), 1e-4, 1.0);
    const float noh = clamp(dot(n, h), 0.0, 1.0);
    const float alpha = max(roughness * roughness, 2e-3);
    const vec3 f0 = mix(vec3(0.08 * specular), albedo, metallic);
    const vec3 fresnel = ayloFresnel(f0, dot(v, h));
    diffuse = (1.0 - fresnel) * (1.0 - metallic) * albedo / AYLO_PI * nol;
    reflected = fresnel * ayloDistribution(noh, alpha) * ayloVisibility(nol, nov, alpha) * nol;
}

// Karis's analytic approximation of the split-sum environment BRDF: how much of a uniform surrounding light the specular
// lobe takes, for the ambient light.
vec3 ayloEnvironmentBrdf(vec3 f0, float roughness, float nov) {
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    const vec4 r = (roughness * c0) + c1;
    const float a004 = (min(r.x * r.x, exp2(-9.28 * nov)) * r.x) + r.y;
    const vec2 ab = (vec2(-1.04, 1.04) * a004) + r.zw;
    return (f0 * ab.x) + ab.y;
}

// The fall-off of a point or spot light with its distance d, kept finite near it and brought to 0 at its range (0: none),
// as glTF's KHR_lights_punctual recommends.
float ayloDistanceFalloff(float d, float range) {
    float falloff = 1.0 / max(d * d, 1e-4);
    if (range > 0.0) {
        const float ratio = d / range;
        const float window = clamp(1.0 - (ratio * ratio * ratio * ratio), 0.0, 1.0);
        falloff *= window * window;
    }
    return falloff;
}

// The linear depth (distance along the light's view) of a depth `ndc` of shadow map `tile`.
float ayloShadowLinear(AyloShadowTile tile, float ndc) {
    const float near = tile.depth.x;
    const float far = tile.depth.y;
    return tile.depth.z > 0.5 ? (near * far) / max(far - (ndc * (far - near)), 1e-6) : near + (ndc * (far - near));
}

// 16 points of a Poisson disc, for the blocker search and the filter.
const vec2 AYLO_POISSON[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725), vec2(-0.09418410, -0.92938870),
    vec2(0.34495938, 0.29387760), vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379), vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420), vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590), vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790));

// The depth of the atlas's texel at `at` (0 to 1), kept between `lowest` and `highest`: read by texelFetch, the texel
// as it is, so no sampler goes through the calls (which some translations to other APIs lose).
float ayloShadowTexel(vec2 at, vec2 lowest, vec2 highest) {
    const vec2 size = vec2(textureSize(ayloShadowAtlas, 0));
    return texelFetch(ayloShadowAtlas, ivec2(clamp(at, lowest, highest) * size), 0).r;
}

// How lit `position` (normal n) is by shadow map `index`: 1 lit, 0 in shadow, between in a penumbra. Percentage-closer
// soft shadows: the blockers found around it give a penumbra as wide as the light's size makes it, filtered by PCF.
float ayloShadowMap(uint index, vec3 position, vec3 n) {
    const AyloShadowTile tile = ayloShadows.tile[index];
    const float texels = tile.rect.z / tile.size.z;  // the map's side, in texels
    vec4 clip = tile.viewProjection * vec4(position, 1.0);
    const float distance = tile.depth.z > 0.5 ? max(clip.w, 1e-4) : 1.0;
    const float texelWorld = tile.depth.z > 0.5 ? (2.0 * tile.size.x * distance / texels) : (tile.size.x / texels);
    // Along the normal by a few texels: a surface does not shadow itself (acne), whatever its slope.
    clip = tile.viewProjection * vec4(position + (n * tile.bias.y * texelWorld * 1.5), 1.0);
    const vec3 ndc = clip.xyz / clip.w;
    const vec2 uv = (ndc.xy * 0.5) + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) || ndc.z > 1.0 || ndc.z < 0.0) {
        return 1.0;
    }
    const float receiver = ayloShadowLinear(tile, ndc.z) - (tile.bias.x * texelWorld);
    const vec2 lowest = tile.rect.xy + (0.5 * tile.size.z);
    const vec2 highest = tile.rect.xy + tile.rect.zw - (0.5 * tile.size.z);
    const vec2 at = tile.rect.xy + (uv * tile.rect.zw);
    // The blockers: what is nearer to the light than the receiver, around it.
    const float search = 8.0 * tile.size.z;
    float blockers = 0.0;
    float blockerDepth = 0.0;
    for (int i = 0; i < 16; ++i) {
        const float depth = ayloShadowLinear(tile, ayloShadowTexel(at + (AYLO_POISSON[i] * search), lowest, highest));
        if (depth < receiver) {
            blockers += 1.0;
            blockerDepth += depth;
        }
    }
    if (blockers == 0.0) {
        return 1.0;
    }
    blockerDepth /= blockers;
    // The penumbra grows with the gap between the blocker and the receiver, and the light's size.
    const float penumbraWorld = tile.depth.z > 0.5 ? (receiver - blockerDepth) * tile.size.y / max(blockerDepth, 1e-3)
                                                   : (receiver - blockerDepth) * tile.size.y * 2.0;
    const float radius = clamp(penumbraWorld / texelWorld, 1.0, 12.0) * tile.size.z;
    float lit = 0.0;
    for (int i = 0; i < 16; ++i) {
        const float depth = ayloShadowLinear(tile, ayloShadowTexel(at + (AYLO_POISSON[i] * radius), lowest, highest));
        lit += depth >= receiver ? 1.0 : 0.0;
    }
    return lit / 16.0;
}

// How lit `position` is by light `index`: its shadow map (the cascade at that distance from the camera, the cube face
// it is in) if it casts shadows and the model receives them, else 1.
float ayloShadowOf(uint index, vec3 position, vec3 n) {
    const AyloLight light = ayloLights.light[index];
    const uint first = uint(light.right.w + 0.5);
    if (first == 0u || ayloMeshDraws[ayloPart.draw].boundsMin.w < 0.5) {
        return 1.0;
    }
    const uint kind = uint(light.direction.w + 0.5);
    uint tile = first - 1u;
    if (kind == 0u) {
        const float depth = -(ayloFrame.view * vec4(position, 1.0)).z;
        uint cascade = 0u;
        while (cascade < ayloLights.count.y && depth > ayloShadows.tile[tile + cascade].depth.w) {
            ++cascade;
        }
        if (cascade == ayloLights.count.y) {
            return 1.0;  // past the shadow distance
        }
        tile += cascade;
    } else if (kind == 1u) {
        const vec3 away = position - light.position.xyz;
        const vec3 size = abs(away);
        if (size.x >= size.y && size.x >= size.z) {
            tile += away.x > 0.0 ? 0u : 1u;
        } else if (size.y >= size.z) {
            tile += away.y > 0.0 ? 2u : 3u;
        } else {
            tile += away.z > 0.0 ? 4u : 5u;
        }
    }
    return ayloShadowMap(tile, position, n);
}

// The irradiance a rectangle of radiance 1, of corners c[4] around the point (relative to it), gives a surface of
// normal n: its form factor times pi, by Lambert's formula, edge by edge (exact while the rectangle is above the
// surface's horizon). Which side of the rectangle shines is checked by the caller.
float ayloRectangleIrradiance(vec3 n, vec3 c[4]) {
    float sum = 0.0;
    for (int i = 0; i < 4; ++i) {
        const vec3 a = normalize(c[i]);
        const vec3 b = normalize(c[(i + 1) % 4]);
        const float cosine = clamp(dot(a, b), -0.9999, 0.9999);
        sum += acos(cosine) * dot(normalize(cross(a, b)), n);
    }
    return max(abs(0.5 * sum) * sign(dot(n, c[0] + c[1] + c[2] + c[3])), 0.0);
}
