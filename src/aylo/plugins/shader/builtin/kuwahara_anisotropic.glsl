/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** kuwahara_anisotropic
*/

// The anisotropic Kuwahara filter (Kyprianidis, Kang and Doellner 2009, with the polynomial sector weights of
// Kyprianidis 2011): the classic filter's squares become eight sectors of an ellipse turned along the picture's
// strokes, so the patches follow its shapes, like brush strokes. `tensor` is the blurred structure tensor.
#pragma screen

uniform sampler2D tensor;        // The structure tensor, blurred (engine.effects.kuwahara binds it).
uniform float radius = 6.0;      // The ellipse's size, in pixels.
uniform float sharpness = 8.0;   // How strongly the least varied sectors win: higher, crisper edges.
uniform float eccentricity = 1.0;  // How much the ellipse stretches along strokes: lower, more stretched.

void fragment() {
    vec3 structure = texture(tensor, SCREEN_UV).rgb;
    float e = structure.r;
    float g = structure.g;
    float f = structure.b;
    float root = sqrt(((e - g) * (e - g)) + (4.0 * f * f));
    float lambda1 = 0.5 * (e + g + root);
    float lambda2 = 0.5 * (e + g - root);
    vec2 along = vec2(lambda1 - e, -f);
    along = length(along) > 0.0 ? normalize(along) : vec2(0.0, 1.0);
    float anisotropy = (lambda1 + lambda2) > 0.0 ? (lambda1 - lambda2) / (lambda1 + lambda2) : 0.0;
    float angle = atan(along.y, along.x);
    float a = radius * clamp((eccentricity + anisotropy) / eccentricity, 0.1, 2.0);
    float b = radius * clamp(eccentricity / (eccentricity + anisotropy), 0.1, 2.0);
    float cosine = cos(angle);
    float sine = sin(angle);
    // Pixels to the unit disc of the sectors: rotate against the strokes, then squash the ellipse round.
    mat2 toDisc = mat2(cosine / a, -sine / b, sine / a, cosine / b);
    int reachX = int(ceil(sqrt((a * a * cosine * cosine) + (b * b * sine * sine))));
    int reachY = int(ceil(sqrt((a * a * sine * sine) + (b * b * cosine * cosine))));

    const float zeta = 0.33;
    const float eta = (zeta + 0.92387953) / (0.38268343 * 0.38268343);  // (zeta + cos(pi / 8)) / sin(pi / 8)^2
    vec4 means[8];
    vec3 squares[8];
    for (int k = 0; k < 8; ++k) {
        means[k] = vec4(0.0);
        squares[k] = vec3(0.0);
    }
    for (int y = -reachY; y <= reachY; ++y) {
        for (int x = -reachX; x <= reachX; ++x) {
            vec2 v = toDisc * vec2(float(x), float(y));
            if (dot(v, v) > 1.0) {
                continue;
            }
            vec3 color = texture(TEXTURE, SCREEN_UV + (vec2(float(x), float(y)) * SCREEN_PIXEL_SIZE)).rgb;
            float weights[8];
            float total = 0.0;
            for (int turn = 0; turn < 2; ++turn) {
                vec2 w = turn == 0 ? v : 0.70710678 * vec2(v.x - v.y, v.x + v.y);
                float xx = zeta - (eta * w.x * w.x);
                float yy = zeta - (eta * w.y * w.y);
                float z0 = max(0.0, w.y + xx);
                float z2 = max(0.0, -w.x + yy);
                float z4 = max(0.0, -w.y + xx);
                float z6 = max(0.0, w.x + yy);
                weights[turn] = z0 * z0;
                weights[2 + turn] = z2 * z2;
                weights[4 + turn] = z4 * z4;
                weights[6 + turn] = z6 * z6;
                total += (z0 * z0) + (z2 * z2) + (z4 * z4) + (z6 * z6);
            }
            float falloff = exp(-3.125 * dot(v, v)) / max(total, 1e-6);
            for (int k = 0; k < 8; ++k) {
                float weight = weights[k] * falloff;
                means[k] += vec4(color * weight, weight);
                squares[k] += color * color * weight;
            }
        }
    }
    vec4 result = vec4(0.0);
    for (int k = 0; k < 8; ++k) {
        if (means[k].w <= 0.0) {
            continue;
        }
        vec3 mean = means[k].rgb / means[k].w;
        vec3 variance = abs((squares[k] / means[k].w) - (mean * mean));
        float spread = variance.r + variance.g + variance.b;
        float weight = 1.0 / (1.0 + pow(255.0 * spread, 0.5 * sharpness));
        result += vec4(mean * weight, weight);
    }
    if (result.w > 0.0) {
        COLOR.rgb = result.rgb / result.w;
    }
}
