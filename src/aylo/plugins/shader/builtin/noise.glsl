// @aylo/noise.glsl: hashes, white, value, Perlin and simplex noise in 1D to 4D, fractal Brownian motion and Voronoi.
// Written in the part of GLSL that C++ also reads (see @aylo/math.glsl). The noises are continuous and the same on every
// GPU: a position gives the same value everywhere.

// --- Hashes: well-mixed bits of integers (PCG, O'Neill 2014), and a number in [0, 1) from them.

uint hashBits(uint value) {
    uint state = (value * 747796405u) + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}
uint hashBits(uvec2 value) {
    return hashBits(value.x ^ hashBits(value.y));
}
uint hashBits(uvec3 value) {
    return hashBits(value.x ^ hashBits(value.y ^ hashBits(value.z)));
}
uint hashBits(uvec4 value) {
    return hashBits(value.x ^ hashBits(value.y ^ hashBits(value.z ^ hashBits(value.w))));
}
float hashToFloat(uint bits) {
    return float(bits >> 8u) / 16777216.0;
}

// --- White noise: a number in [0, 1) for each position, unrelated to its neighbours'.

float whiteNoise(float position) {
    return hashToFloat(hashBits(floatBitsToUint(position)));
}
float whiteNoise(vec2 position) {
    return hashToFloat(hashBits(uvec2(floatBitsToUint(position.x), floatBitsToUint(position.y))));
}
float whiteNoise(vec3 position) {
    return hashToFloat(
        hashBits(uvec3(floatBitsToUint(position.x), floatBitsToUint(position.y), floatBitsToUint(position.z))));
}
float whiteNoise(vec4 position) {
    return hashToFloat(hashBits(uvec4(floatBitsToUint(position.x), floatBitsToUint(position.y),
                                      floatBitsToUint(position.z), floatBitsToUint(position.w))));
}

// The quintic curve that blends between lattice points (Perlin 2002): smooth to the second derivative.
float noiseFade(float t) {
    return t * t * t * ((t * ((t * 6.0) - 15.0)) + 10.0);
}

// --- Value noise: random values at the integer points, blended smoothly. In [0, 1].

float valueNoise(float position) {
    float cell = floor(position);
    int base = int(cell);
    float low = hashToFloat(hashBits(uint(base)));
    float high = hashToFloat(hashBits(uint(base + 1)));
    return mix(low, high, noiseFade(position - cell));
}
float valueNoise(vec2 position) {
    vec2 cell = floor(position);
    vec2 inside = position - cell;
    vec2 fade = vec2(noiseFade(inside.x), noiseFade(inside.y));
    ivec2 base = ivec2(cell);
    float result = 0.0;
    for (int corner = 0; corner < 4; corner++) {
        ivec2 offset = ivec2(corner & 1, (corner >> 1) & 1);
        vec2 weights = mix(vec2(1.0) - fade, fade, vec2(offset));
        result += hashToFloat(hashBits(uvec2(base + offset))) * weights.x * weights.y;
    }
    return result;
}
float valueNoise(vec3 position) {
    vec3 cell = floor(position);
    vec3 inside = position - cell;
    vec3 fade = vec3(noiseFade(inside.x), noiseFade(inside.y), noiseFade(inside.z));
    ivec3 base = ivec3(cell);
    float result = 0.0;
    for (int corner = 0; corner < 8; corner++) {
        ivec3 offset = ivec3(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1);
        vec3 weights = mix(vec3(1.0) - fade, fade, vec3(offset));
        result += hashToFloat(hashBits(uvec3(base + offset))) * weights.x * weights.y * weights.z;
    }
    return result;
}
float valueNoise(vec4 position) {
    vec4 cell = floor(position);
    vec4 inside = position - cell;
    vec4 fade = vec4(noiseFade(inside.x), noiseFade(inside.y), noiseFade(inside.z), noiseFade(inside.w));
    ivec4 base = ivec4(cell);
    float result = 0.0;
    for (int corner = 0; corner < 16; corner++) {
        ivec4 offset = ivec4(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1, (corner >> 3) & 1);
        vec4 weights = mix(vec4(1.0) - fade, fade, vec4(offset));
        result += hashToFloat(hashBits(uvec4(base + offset))) * weights.x * weights.y * weights.z * weights.w;
    }
    return result;
}

// --- The random gradient of a lattice point: a slope in 1D, a unit vector above.

float noiseGradient(int cell) {
    return (hashToFloat(hashBits(uint(cell))) * 2.0) - 1.0;
}
vec2 noiseGradient(ivec2 cell) {
    float angle = hashToFloat(hashBits(uvec2(cell))) * 6.28318530717959;
    return vec2(cos(angle), sin(angle));
}
vec3 noiseGradient(ivec3 cell) {
    uint bits = hashBits(uvec3(cell));
    float z = (hashToFloat(bits) * 2.0) - 1.0;
    float angle = hashToFloat(hashBits(bits)) * 6.28318530717959;
    float ring = sqrt(max(1.0 - (z * z), 0.0));
    return vec3(ring * cos(angle), ring * sin(angle), z);
}
vec4 noiseGradient(ivec4 cell) {
    uint first = hashBits(uvec4(cell));
    uint second = hashBits(first);
    uint third = hashBits(second);
    uint fourth = hashBits(third);
    vec4 direction =
        (vec4(hashToFloat(first), hashToFloat(second), hashToFloat(third), hashToFloat(fourth)) * 2.0) - 1.0;
    return direction / max(length(direction), 0.0001);
}

// --- Perlin (gradient) noise: 0 at every integer point, smooth hills and valleys between. About -1 to 1.

float perlinNoise(float position) {
    float cell = floor(position);
    float inside = position - cell;
    int base = int(cell);
    float low = noiseGradient(base) * inside;
    float high = noiseGradient(base + 1) * (inside - 1.0);
    return mix(low, high, noiseFade(inside)) * 2.0;
}
float perlinNoise(vec2 position) {
    vec2 cell = floor(position);
    vec2 inside = position - cell;
    vec2 fade = vec2(noiseFade(inside.x), noiseFade(inside.y));
    ivec2 base = ivec2(cell);
    float result = 0.0;
    for (int corner = 0; corner < 4; corner++) {
        ivec2 offset = ivec2(corner & 1, (corner >> 1) & 1);
        vec2 weights = mix(vec2(1.0) - fade, fade, vec2(offset));
        result += dot(noiseGradient(base + offset), inside - vec2(offset)) * weights.x * weights.y;
    }
    return result * 1.41421356;
}
float perlinNoise(vec3 position) {
    vec3 cell = floor(position);
    vec3 inside = position - cell;
    vec3 fade = vec3(noiseFade(inside.x), noiseFade(inside.y), noiseFade(inside.z));
    ivec3 base = ivec3(cell);
    float result = 0.0;
    for (int corner = 0; corner < 8; corner++) {
        ivec3 offset = ivec3(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1);
        vec3 weights = mix(vec3(1.0) - fade, fade, vec3(offset));
        result += dot(noiseGradient(base + offset), inside - vec3(offset)) * weights.x * weights.y * weights.z;
    }
    return result * 1.15470054;
}
float perlinNoise(vec4 position) {
    vec4 cell = floor(position);
    vec4 inside = position - cell;
    vec4 fade = vec4(noiseFade(inside.x), noiseFade(inside.y), noiseFade(inside.z), noiseFade(inside.w));
    ivec4 base = ivec4(cell);
    float result = 0.0;
    for (int corner = 0; corner < 16; corner++) {
        ivec4 offset = ivec4(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1, (corner >> 3) & 1);
        vec4 weights = mix(vec4(1.0) - fade, fade, vec4(offset));
        result += dot(noiseGradient(base + offset), inside - vec4(offset)) * weights.x * weights.y * weights.z *
                  weights.w;
    }
    return result;
}

// --- Simplex noise (Perlin 2001): gradients on the corners of a simplex grid, cheaper than Perlin noise above 2D and
// without its axis-aligned look. About -1 to 1.

float simplexNoise(float position) {
    float cell = floor(position);
    float result = 0.0;
    for (int corner = 0; corner < 2; corner++) {
        float offset = position - (cell + float(corner));
        float falloff = max(1.0 - (offset * offset), 0.0);
        falloff *= falloff;
        result += falloff * falloff * noiseGradient(int(cell) + corner) * offset;
    }
    return result * 2.4;
}
float simplexNoise(vec2 position) {
    const float skew = 0.366025403784439;    // (sqrt(3) - 1) / 2
    const float unskew = 0.211324865405187;  // (3 - sqrt(3)) / 6
    vec2 skewed = position + vec2((position.x + position.y) * skew);
    vec2 cell = floor(skewed);
    vec2 inside = skewed - cell;
    vec2 origin = position - (cell - vec2((cell.x + cell.y) * unskew));
    ivec2 base = ivec2(cell);
    ivec2 rank = ivec2(inside.x > inside.y ? 0 : 1, inside.x > inside.y ? 1 : 0);
    float result = 0.0;
    for (int corner = 0; corner < 3; corner++) {
        ivec2 offset = ivec2(rank.x < corner ? 1 : 0, rank.y < corner ? 1 : 0);
        vec2 toCorner = origin - vec2(offset) + vec2(float(corner) * unskew);
        float falloff = max(0.5 - dot(toCorner, toCorner), 0.0);
        falloff *= falloff;
        result += falloff * falloff * dot(noiseGradient(base + offset), toCorner);
    }
    return result * 70.0;
}
float simplexNoise(vec3 position) {
    const float skew = 0.333333333333333;
    const float unskew = 0.166666666666667;
    vec3 skewed = position + vec3((position.x + position.y + position.z) * skew);
    vec3 cell = floor(skewed);
    vec3 inside = skewed - cell;
    vec3 origin = position - (cell - vec3((cell.x + cell.y + cell.z) * unskew));
    ivec3 base = ivec3(cell);
    ivec3 rank = ivec3(0);
    for (int axis = 0; axis < 3; axis++) {
        for (int other = 0; other < 3; other++) {
            if (other != axis && (inside[other] > inside[axis] || (inside[other] == inside[axis] && other < axis))) {
                rank[axis] += 1;
            }
        }
    }
    float result = 0.0;
    for (int corner = 0; corner < 4; corner++) {
        ivec3 offset = ivec3(rank.x < corner ? 1 : 0, rank.y < corner ? 1 : 0, rank.z < corner ? 1 : 0);
        vec3 toCorner = origin - vec3(offset) + vec3(float(corner) * unskew);
        float falloff = max(0.6 - dot(toCorner, toCorner), 0.0);
        falloff *= falloff;
        result += falloff * falloff * dot(noiseGradient(base + offset), toCorner);
    }
    return result * 32.0;
}
float simplexNoise(vec4 position) {
    const float skew = 0.309016994374947;    // (sqrt(5) - 1) / 4
    const float unskew = 0.138196601125011;  // (5 - sqrt(5)) / 20
    vec4 skewed = position + vec4((position.x + position.y + position.z + position.w) * skew);
    vec4 cell = floor(skewed);
    vec4 inside = skewed - cell;
    vec4 origin = position - (cell - vec4((cell.x + cell.y + cell.z + cell.w) * unskew));
    ivec4 base = ivec4(cell);
    ivec4 rank = ivec4(0);
    for (int axis = 0; axis < 4; axis++) {
        for (int other = 0; other < 4; other++) {
            if (other != axis && (inside[other] > inside[axis] || (inside[other] == inside[axis] && other < axis))) {
                rank[axis] += 1;
            }
        }
    }
    float result = 0.0;
    for (int corner = 0; corner < 5; corner++) {
        ivec4 offset = ivec4(rank.x < corner ? 1 : 0, rank.y < corner ? 1 : 0, rank.z < corner ? 1 : 0,
                             rank.w < corner ? 1 : 0);
        vec4 toCorner = origin - vec4(offset) + vec4(float(corner) * unskew);
        float falloff = max(0.6 - dot(toCorner, toCorner), 0.0);
        falloff *= falloff;
        result += falloff * falloff * dot(noiseGradient(base + offset), toCorner);
    }
    return result * 27.0;
}

// --- Fractal Brownian motion: octaves of Perlin noise, each gain times as strong and lacunarity times as fine as the
// one before, divided by their total strength. About -1 to 1. Usual values: 4 to 6 octaves, gain 0.5, lacunarity 2.

float fbm(float position, int octaves, float gain, float lacunarity) {
    float sum = 0.0;
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    for (int octave = 0; octave < octaves; octave++) {
        sum += amplitude * perlinNoise((position * frequency) + (float(octave) * 19.19));
        total += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return total > 0.0 ? sum / total : 0.0;
}
float fbm(vec2 position, int octaves, float gain, float lacunarity) {
    float sum = 0.0;
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    for (int octave = 0; octave < octaves; octave++) {
        sum += amplitude * perlinNoise((position * frequency) + vec2(float(octave) * 19.19));
        total += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return total > 0.0 ? sum / total : 0.0;
}
float fbm(vec3 position, int octaves, float gain, float lacunarity) {
    float sum = 0.0;
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    for (int octave = 0; octave < octaves; octave++) {
        sum += amplitude * perlinNoise((position * frequency) + vec3(float(octave) * 19.19));
        total += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return total > 0.0 ? sum / total : 0.0;
}
float fbm(vec4 position, int octaves, float gain, float lacunarity) {
    float sum = 0.0;
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    for (int octave = 0; octave < octaves; octave++) {
        sum += amplitude * perlinNoise((position * frequency) + vec4(float(octave) * 19.19));
        total += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return total > 0.0 ? sum / total : 0.0;
}

// --- Voronoi (Worley 1996): one random point in each cell of the lattice. voronoi() returns the distance to the
// nearest point (F1), to the second nearest (F2), and F2 - F1, which is 0 on the border between two cells.

const int VORONOI_EUCLIDEAN = 0;  // the straight-line distance: round cells
const int VORONOI_MANHATTAN = 1;  // |dx| + |dy|: diamond-shaped cells
const int VORONOI_CHEBYSHEV = 2;  // max(|dx|, |dy|): square cells
const int VORONOI_MINKOWSKI = 3;  // (|dx|^e + |dy|^e)^(1/e): 1 is Manhattan, 2 Euclidean, large Chebyshev

float voronoiDistance(vec2 offset, int metric, float exponent) {
    if (metric == VORONOI_MANHATTAN) {
        return abs(offset.x) + abs(offset.y);
    }
    if (metric == VORONOI_CHEBYSHEV) {
        return max(abs(offset.x), abs(offset.y));
    }
    if (metric == VORONOI_MINKOWSKI) {
        return pow(pow(abs(offset.x), exponent) + pow(abs(offset.y), exponent), 1.0 / exponent);
    }
    return length(offset);
}
float voronoiDistance(vec3 offset, int metric, float exponent) {
    if (metric == VORONOI_MANHATTAN) {
        return abs(offset.x) + abs(offset.y) + abs(offset.z);
    }
    if (metric == VORONOI_CHEBYSHEV) {
        return max(abs(offset.x), max(abs(offset.y), abs(offset.z)));
    }
    if (metric == VORONOI_MINKOWSKI) {
        return pow(pow(abs(offset.x), exponent) + pow(abs(offset.y), exponent) + pow(abs(offset.z), exponent),
                   1.0 / exponent);
    }
    return length(offset);
}

vec3 voronoi(vec2 position, int metric, float exponent) {
    ivec2 base = ivec2(floor(position));
    float nearest = 1e9;
    float second = 1e9;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            ivec2 neighbour = base + ivec2(x, y);
            uint bits = hashBits(uvec3(uvec2(neighbour), 7u));
            vec2 feature = vec2(neighbour) + vec2(hashToFloat(bits), hashToFloat(hashBits(bits)));
            float measured = voronoiDistance(feature - position, metric, exponent);
            if (measured < nearest) {
                second = nearest;
                nearest = measured;
            } else if (measured < second) {
                second = measured;
            }
        }
    }
    return vec3(nearest, second, second - nearest);
}
vec3 voronoi(vec3 position, int metric, float exponent) {
    ivec3 base = ivec3(floor(position));
    float nearest = 1e9;
    float second = 1e9;
    for (int z = -1; z <= 1; z++) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                ivec3 neighbour = base + ivec3(x, y, z);
                uint bits = hashBits(uvec4(uvec3(neighbour), 7u));
                uint more = hashBits(bits);
                vec3 feature =
                    vec3(neighbour) + vec3(hashToFloat(bits), hashToFloat(more), hashToFloat(hashBits(more)));
                float measured = voronoiDistance(feature - position, metric, exponent);
                if (measured < nearest) {
                    second = nearest;
                    nearest = measured;
                } else if (measured < second) {
                    second = measured;
                }
            }
        }
    }
    return vec3(nearest, second, second - nearest);
}

// The Euclidean Voronoi, the usual one.
vec3 voronoi(vec2 position) {
    return voronoi(position, VORONOI_EUCLIDEAN, 2.0);
}
vec3 voronoi(vec3 position) {
    return voronoi(position, VORONOI_EUCLIDEAN, 2.0);
}
