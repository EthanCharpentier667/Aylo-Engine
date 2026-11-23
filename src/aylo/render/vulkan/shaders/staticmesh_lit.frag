/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** staticmesh_lit
*/

// The default shader of static meshes in a frame that has lights, fragment stage: the shaders ABI with no game code,
// lit (AYLO_LIT), the part's colours and texture as its albedo, as a `#pragma lit` shader that writes nothing.
#version 450
#extension GL_GOOGLE_include_directive : require

#define AYLO_MESH
#define AYLO_BUILTIN_MESH
#define AYLO_LIT
#include "common.glsl"
#include "fragment_common.glsl"
#include "lighting.glsl"
#include "mesh.frag.glsl"
