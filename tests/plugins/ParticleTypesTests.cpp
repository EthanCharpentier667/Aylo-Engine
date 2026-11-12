/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ParticleTypesTests
*/

#include <gtest/gtest.h>

#include <string_view>

#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "plugins/particle/ParticleEmitter.hpp"
#include "plugins/particle/ParticleEvents.hpp"

using aylo::ecs::FieldType;
using aylo::plugins::particle::ParticleEmitter;
using aylo::plugins::particle::ParticlesFinished;
using aylo::plugins::particle::SpawnParticles;

static_assert(aylo::ecs::Component<ParticleEmitter>);
static_assert(aylo::ecs::Event<SpawnParticles>);
static_assert(aylo::ecs::Event<ParticlesFinished>);

namespace {
/// @return The reflected type of the field @p name of @p T, failing the test if it is not reflected.
template <typename T>
FieldType getFieldType(std::string_view name) {
    for (const auto& field : aylo::ecs::TypeTraits<T>::kFields) {
        if (field.name == name) {
            return field.type;
        }
    }
    ADD_FAILURE() << "No reflected field '" << name << "'";
    return FieldType::kBool;
}
}  // namespace

TEST(ParticleTypesTests, EmitterReflectsEveryField) {
    EXPECT_EQ(aylo::ecs::TypeTraits<ParticleEmitter>::kName, "aylo.ParticleEmitter");
    EXPECT_EQ(aylo::ecs::TypeTraits<ParticleEmitter>::kFields.size(), 8U);
    EXPECT_EQ(getFieldType<ParticleEmitter>("effect"), FieldType::kAssetId);
    EXPECT_EQ(getFieldType<ParticleEmitter>("simulation"), FieldType::kU8);
    EXPECT_EQ(getFieldType<ParticleEmitter>("seed"), FieldType::kU32);
}

TEST(ParticleTypesTests, EmitterDefaultsPlayTheEffectsChoice) {
    const ParticleEmitter emitter;
    EXPECT_TRUE(emitter.playing);
    EXPECT_FLOAT_EQ(emitter.rateScale, 1.0F);
    EXPECT_FALSE(emitter.overrideSimulation);
    EXPECT_EQ(emitter.seed, 0U);
}

TEST(ParticleTypesTests, EventsReflectTheirFields) {
    EXPECT_EQ(aylo::ecs::TypeTraits<SpawnParticles>::kName, "aylo.SpawnParticles");
    EXPECT_EQ(getFieldType<SpawnParticles>("effect"), FieldType::kAssetId);
    EXPECT_EQ(getFieldType<SpawnParticles>("position"), FieldType::kVec2);
    EXPECT_EQ(getFieldType<SpawnParticles>("layer"), FieldType::kI32);
    EXPECT_EQ(aylo::ecs::TypeTraits<ParticlesFinished>::kName, "aylo.ParticlesFinished");
    EXPECT_EQ(getFieldType<ParticlesFinished>("entity"), FieldType::kEntity);
}
