/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** staticmesh
*/

// The default shader of static meshes, fragment stage, unlit: the shaders ABI (contract/renderer/shaders/) with no game
// code, the stage game shaders are assembled in for kMesh.
#version 450
#extension GL_GOOGLE_include_directive : require

#define AYLO_MESH
#include "common.glsl"
#include "fragment_common.glsl"
#include "mesh.frag.glsl"
