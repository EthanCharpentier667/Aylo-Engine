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
