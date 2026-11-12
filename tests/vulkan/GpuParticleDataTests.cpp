/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GpuParticleDataTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>

#include "aylo/contract/renderer/exceptions/ParticleExceptions.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/contract/renderer/particle/ParticleCurve.hpp"
#include "aylo/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "render/vulkan/particle/GpuParticleData.hpp"

using aylo::contract::renderer::CircleShape;
using aylo::contract::renderer::ParticleSpace;
using aylo::contract::renderer::ParticleSystemDesc;
using aylo::contract::renderer::RectShape;
using aylo::render::vulkan::particle::GpuParticleParams;

TEST(GpuParticleParams, CopiesTheRangesAndForces) {
    ParticleSystemDesc desc;
    desc.capacity = 512;
    desc.lifetime = {.min = 0.5F, .max = 2.0F};
    desc.speed = {.min = 10.0F, .max = 20.0F};
    desc.size = {.min = 3.0F, .max = 4.0F};
    desc.gravity = {1.0F, 98.0F};
    desc.drag = 0.25F;
    desc.space = ParticleSpace::kLocal;
    desc.color = {0.5F, 0.25F, 1.0F, 0.75F};
    const GpuParticleParams params = GpuParticleParams::fromDesc(desc);
    EXPECT_EQ(params.capacity, 512U);
    EXPECT_EQ(params.lifetime, (glm::vec2{0.5F, 2.0F}));
    EXPECT_EQ(params.speed, (glm::vec2{10.0F, 20.0F}));
    EXPECT_EQ(params.size, (glm::vec2{3.0F, 4.0F}));
    EXPECT_EQ(params.gravity, (glm::vec2{1.0F, 98.0F}));
    EXPECT_FLOAT_EQ(params.drag, 0.25F);
    EXPECT_EQ(params.space, 1U);
    EXPECT_EQ(params.color, (glm::vec4{0.5F, 0.25F, 1.0F, 0.75F}));
}

TEST(GpuParticleParams, ReadsEachShape) {
    ParticleSystemDesc desc;
    EXPECT_EQ(GpuParticleParams::fromDesc(desc).shape, aylo::render::vulkan::particle::kShapePoint);

    desc.shape = CircleShape{.radius = 5.0F, .edgeOnly = true};
    const GpuParticleParams circle = GpuParticleParams::fromDesc(desc);
    EXPECT_EQ(circle.shape, aylo::render::vulkan::particle::kShapeCircle);
    EXPECT_EQ(circle.shapeSize, (glm::vec2{5.0F, 0.0F}));
    EXPECT_EQ(circle.edgeOnly, 1U);

    desc.shape = RectShape{.size = {8.0F, 2.0F}};
    const GpuParticleParams rect = GpuParticleParams::fromDesc(desc);
    EXPECT_EQ(rect.shape, aylo::render::vulkan::particle::kShapeRect);
    EXPECT_EQ(rect.shapeSize, (glm::vec2{8.0F, 2.0F}));
}

TEST(GpuParticleParams, BakesTheCurves) {
    ParticleSystemDesc desc;
    desc.sizeOverLife.keys = {{.t = 0.0F, .value = 2.0F}, {.t = 1.0F, .value = 0.0F}};
    const GpuParticleParams params = GpuParticleParams::fromDesc(desc);
    EXPECT_EQ(params.sizeOverLife, aylo::contract::renderer::bakeCurve(desc.sizeOverLife));
    EXPECT_FLOAT_EQ(params.speedOverLife.front(), 1.0F);
    EXPECT_EQ(params.colorOverLife.back(), (glm::vec4{1.0F}));
}

TEST(GpuParticleParams, ThrowsOnAnInvalidCurve) {
    ParticleSystemDesc desc;
    desc.sizeOverLife.keys = {{.t = 2.0F, .value = 1.0F}};
    EXPECT_THROW(static_cast<void>(GpuParticleParams::fromDesc(desc)),
                 aylo::contract::renderer::exceptions::ParticleException);
}
