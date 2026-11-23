/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** staticmesh
*/

// The default shader of static meshes, vertex stage: the shaders ABI (contract/renderer/shaders/) with no game code,
// the stage game shaders are assembled in for kMesh, so built-in and game meshes are drawn alike.
#version 450
#extension GL_GOOGLE_include_directive : require

#define AYLO_MESH
#define AYLO_BUILTIN_MESH
#include "common.glsl"
#include "mesh.vert.glsl"
