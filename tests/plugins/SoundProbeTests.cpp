/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SoundProbeTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "SoundFiles.hpp"
#include "plugins/audio/SoundProbe.hpp"

namespace {
using aylo::plugins::audio::probeSound;
using aylo::plugins::audio::SoundInfo;
using aylo::plugins::audio::SoundProbe;

/// @return The info @p probe found, failing the test with its error if it found none.
[[nodiscard]] SoundInfo infoOf(const SoundProbe& probe) {
    EXPECT_TRUE(probe.info) << probe.error;
    return probe.info.value_or(SoundInfo{});
}
}  // namespace

TEST(SoundProbe, ReadsAPcmWav) {
    EXPECT_EQ(infoOf(probeSound(aylo::test::plugins::pcmWav(705600, 1, 44100))),
              (SoundInfo{.sampleRate = 44100, .channels = 1, .frames = 705600}));
    EXPECT_EQ(infoOf(probeSound(aylo::test::plugins::pcmWav(1000, 2, 48000))),
              (SoundInfo{.sampleRate = 48000, .channels = 2, .frames = 1000}));
}

TEST(SoundProbe, ReadsAFloatWav) {
    const auto wav = aylo::test::plugins::wavFile(
        {.format = 3, .channels = 2, .sampleRate = 22050, .bitsPerSample = 32, .blockAlign = 8, .dataSize = 8 * 300});
    EXPECT_EQ(infoOf(probeSound(wav)), (SoundInfo{.sampleRate = 22050, .channels = 2, .frames = 300}));
}

TEST(SoundProbe, CountsTheFramesOfAnAdpcmWavByBlocks) {
    // As dr_wav counts them: two samples a byte after each block's header, plus one in an IMA block's header.
    const auto ima = aylo::test::plugins::wavFile(
        {.format = 0x11, .channels = 1, .bitsPerSample = 4, .blockAlign = 1024, .dataSize = 2048});
    EXPECT_EQ(infoOf(probeSound(ima)).frames, ((2048U - (2U * 4U)) * 2U) + 2U);
    const auto ms = aylo::test::plugins::wavFile(
        {.format = 2, .channels = 2, .bitsPerSample = 4, .blockAlign = 512, .dataSize = 1000});
    EXPECT_EQ(infoOf(probeSound(ms)).frames, (1000U - (2U * 6U * 2U)) * 2U / 2U);
}

TEST(SoundProbe, ReadsTheStreamInfoOfAFlac) {
    std::vector<std::byte> flac = aylo::test::plugins::flacStreamInfo(96000, 2, 0x123456789ULL);
    flac.resize(flac.size() + 100, std::byte{0});
    EXPECT_EQ(infoOf(probeSound(flac)), (SoundInfo{.sampleRate = 96000, .channels = 2, .frames = 0x123456789ULL}));
}

TEST(SoundProbe, ReadsTheLastGranuleOfAnOggVorbis) {
    EXPECT_EQ(infoOf(probeSound(aylo::test::plugins::oggVorbis(44100, 2, 705600))),
              (SoundInfo{.sampleRate = 44100, .channels = 2, .frames = 705600}));
}

TEST(SoundProbe, ReadsAnOggFlac) {
    std::vector<std::byte> mapping{std::byte{0x7F}};
    aylo::test::plugins::putTag(mapping, "FLAC");
    aylo::test::plugins::putBe(mapping, 0x0100, 2);  // mapping version 1.0
    aylo::test::plugins::putBe(mapping, 1, 2);       // header packets
    const std::vector<std::byte> streamInfo = aylo::test::plugins::flacStreamInfo(22050, 1, 0);
    mapping.insert(mapping.end(), streamInfo.begin(), streamInfo.end());
    std::vector<std::byte> ogg;
    aylo::test::plugins::putOggPage(ogg, 0, mapping);
    aylo::test::plugins::putOggPage(ogg, 37485, std::vector<std::byte>(200, std::byte{0}));
    EXPECT_EQ(infoOf(probeSound(ogg)), (SoundInfo{.sampleRate = 22050, .channels = 1, .frames = 37485}));
}

TEST(SoundProbe, CountsTheFramesOfAnMp3) {
    EXPECT_EQ(infoOf(probeSound(aylo::test::plugins::mp3File(10))),
              (SoundInfo{.sampleRate = 44100, .channels = 2, .frames = 11520}));
}

TEST(SoundProbe, LeavesOutTheInfoFrameAndTheEncodersDelayAndPaddingOfAnMp3) {
    EXPECT_EQ(infoOf(probeSound(aylo::test::plugins::mp3File(10, 1105, 300, true))).frames, 11520U - 1105U - 300U);
}

TEST(SoundProbe, SaysWhyItCannotReadAFile) {
    const auto errorOf = [](const std::vector<std::byte>& bytes) {
        const SoundProbe probe = probeSound(bytes);
        EXPECT_FALSE(probe.info);
        return probe.error;
    };
    std::vector<std::byte> text;
    aylo::test::plugins::putTag(text, "hello, world");
    EXPECT_EQ(errorOf(text), "not a sound file the engine decodes (WAV, OGG Vorbis, MP3, FLAC)");
    EXPECT_EQ(errorOf({}), "not a sound file the engine decodes (WAV, OGG Vorbis, MP3, FLAC)");

    std::vector<std::byte> noData = aylo::test::plugins::pcmWav(10);
    noData.resize(12 + 8 + 16);  // RIFF header and fmt chunk only
    EXPECT_EQ(errorOf(noData), "a WAV without a data chunk");
    EXPECT_EQ(errorOf(aylo::test::plugins::wavFile({.format = 0x55, .dataSize = 10})),
              "a WAV in a format the engine does not decode (0x0055)");

    std::vector<std::byte> opusHead;
    aylo::test::plugins::putTag(opusHead, "OpusHead");
    opusHead.resize(19, std::byte{0});
    std::vector<std::byte> opus;
    aylo::test::plugins::putOggPage(opus, 0, opusHead);
    EXPECT_EQ(errorOf(opus), "an Ogg Opus file, which the engine does not decode (use Ogg Vorbis)");

    EXPECT_EQ(errorOf(aylo::test::plugins::flacStreamInfo(44100, 2, 0)), "a FLAC that does not give its length");
}

TEST(SoundProbe, NeverReadsPastTheEndOfATruncatedFile) {
    // Every prefix of every format: whatever it answers, it must not read out of bounds (the sanitizers check).
    const std::vector<std::vector<std::byte>> files{
        aylo::test::plugins::pcmWav(100),
        aylo::test::plugins::flacStreamInfo(44100, 2, 1000),
        aylo::test::plugins::oggVorbis(44100, 2, 1000),
        aylo::test::plugins::mp3File(3, 1105, 300, true),
    };
    for (const std::vector<std::byte>& file : files) {
        for (std::size_t size = 0; size <= file.size(); ++size) {
            const SoundProbe probe = probeSound(std::vector<std::byte>(file.begin(), file.begin() + size));
            EXPECT_TRUE(probe.info || !probe.error.empty());
        }
    }
}
