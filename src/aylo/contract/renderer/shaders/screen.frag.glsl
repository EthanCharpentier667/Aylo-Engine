/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** screen.frag
*/

// The fragment stage of screen effects. TEXTURE is what the previous effect wrote (the scene, for the first one), in
// floating point, so a colour may exceed 1; SCREEN_TEXTURE is another name for it. SCREEN_UV is where this pixel is on
// the screen (0..1, top-left at 0, 0) and SCREEN_PIXEL_SIZE the size of one pixel in it. COLOR starts as the pixel of
// TEXTURE at SCREEN_UV, and what the shader leaves in it is written as is: no blending, the alpha kept. With no game
// code, it copies TEXTURE.
#define SCREEN_TEXTURE TEXTURE
// The 3D world's depth (set 3, binding 0): 0 at the camera's near plane, 1 at its far plane and where nothing 3D was
// drawn. Kept for the effects only when one of them reads it.
layout(set = 3, binding = 0) uniform sampler2D DEPTH_TEXTURE;
#define VIEW_MATRIX ayloFrame.view                         // World to the 3D camera's space.
#define PROJECTION_MATRIX ayloFrame.projection             // The 3D camera's space to clip space.
#define INV_VIEW_MATRIX ayloFrame.inverseView              // The 3D camera's space to the world's.
#define INV_PROJECTION_MATRIX ayloFrame.inverseProjection  // Clip space to the 3D camera's space.
#define CAMERA_POSITION ayloFrame.cameraPosition.xyz       // The 3D camera, in the world.

// The point of the 3D camera's space seen at `uv` (0 to 1 over the screen) with the depth `depth` (DEPTH_TEXTURE's).
vec3 viewPositionAt(vec2 uv, float depth) {
    const vec4 position = ayloFrame.inverseProjection * vec4((uv * 2.0) - 1.0, depth, 1.0);
    return position.xyz / position.w;
}

// The point of the 3D camera's space at `uv`, from DEPTH_TEXTURE.
vec3 viewPositionAt(vec2 uv) {
    return viewPositionAt(uv, texture(DEPTH_TEXTURE, uv).r);
}

// The point of the world at `uv`, from DEPTH_TEXTURE.
vec3 worldPositionAt(vec2 uv) {
    return (ayloFrame.inverseView * vec4(viewPositionAt(uv), 1.0)).xyz;
}

// The distance from the 3D camera's plane to what has the depth `depth` (DEPTH_TEXTURE's), in world units.
float linearDepth(float depth) {
    return -viewPositionAt(vec2(0.5), depth).z;
}

// The normal of the surface at `uv`, in the 3D camera's space (unit, toward the camera), rebuilt from DEPTH_TEXTURE by
// its nearest neighbours: no normal buffer to write.
vec3 normalAt(vec2 uv) {
    const vec2 texel = 1.0 / vec2(textureSize(DEPTH_TEXTURE, 0));
    const vec3 center = viewPositionAt(uv);
    const vec3 right = viewPositionAt(uv + vec2(texel.x, 0.0));
    const vec3 left = viewPositionAt(uv - vec2(texel.x, 0.0));
    const vec3 down = viewPositionAt(uv + vec2(0.0, texel.y));
    const vec3 up = viewPositionAt(uv - vec2(0.0, texel.y));
    // The nearer neighbour on each axis, so an edge does not bend the normal.
    const vec3 alongX = abs(right.z - center.z) < abs(center.z - left.z) ? right - center : center - left;
    const vec3 alongY = abs(down.z - center.z) < abs(center.z - up.z) ? down - center : center - up;
    return normalize(cross(alongY, alongX));
}
vec2 SCREEN_UV;          // This pixel on the screen, 0..1.
vec2 SCREEN_PIXEL_SIZE;  // One pixel, in SCREEN_UV units: 1 / the target's size.

#pragma aylo_shader

void main() {
    UV = ayloUvIn;
    SCREEN_UV = ayloUvIn;
    SCREEN_PIXEL_SIZE = 1.0 / vec2(textureSize(TEXTURE, 0));
    COLOR = texture(TEXTURE, UV);
#pragma aylo_main
    ayloFragColor = COLOR;
}
