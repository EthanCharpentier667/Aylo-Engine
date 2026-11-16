/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioAssetsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "FakeAudioDevice.hpp"
#include "SoundFiles.hpp"
#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/contract/audiodevice/exceptions/AudioExceptions.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/SoundProbe.hpp"

namespace {
/// @brief AudioAssets with a fake device to load them in.
struct AudioAssetsFixture {
    std::shared_ptr<aylo::test::plugins::FakeAudioState> state{std::make_shared<aylo::test::plugins::FakeAudioState>()};
    aylo::test::plugins::FakeAudioDevice device{state};
    aylo::plugins::audio::AudioAssets assets;
};

constexpr std::array<std::byte, 3> kBytes{std::byte{1}, std::byte{2}, std::byte{3}};
}  // namespace

static_assert(aylo::ecs::ResourceTraits<aylo::plugins::audio::AudioAssets>::kName == "aylo.AudioAssets");

TEST(AudioAssets, QueuesASoundAndReturnsTheIdOfItsPath) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load("assets/./sounds/../shot.wav");
    EXPECT_EQ(id, aylo::core::asset::AssetId::fromPath("assets/shot.wav"));
    ASSERT_NE(fixture.assets.get(id), nullptr);
    EXPECT_EQ(fixture.assets.get(id)->path, "assets/shot.wav");
    EXPECT_EQ(fixture.assets.get(id)->state, aylo::plugins::audio::SoundState::kQueued);
    EXPECT_EQ(fixture.assets.getQueued(), std::vector<aylo::core::asset::AssetId>{id});
    EXPECT_FALSE(fixture.assets.getSound(id));
    EXPECT_TRUE(fixture.state->sounds.empty());
}

TEST(AudioAssets, KeepsTheFirstRequestOfAPath) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId first = fixture.assets.load("music.ogg", {.stream = true});
    const aylo::core::asset::AssetId second = fixture.assets.load("music.ogg", {.stream = false});
    EXPECT_EQ(first, second);
    EXPECT_EQ(fixture.assets.getCount(), 1U);
    EXPECT_TRUE(fixture.assets.get(first)->desc.stream);
}

TEST(AudioAssets, LoadsTheQueuedSoundsFromTheirFile) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId shot = fixture.assets.load("shot.wav");
    const aylo::core::asset::AssetId music = fixture.assets.load("music.ogg", {.stream = true});
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    ASSERT_EQ(fixture.state->sounds.size(), 2U);
    EXPECT_EQ(fixture.state->sounds[0].path, "shot.wav");
    EXPECT_EQ(fixture.state->sounds[1].path, "music.ogg");
    EXPECT_TRUE(fixture.state->sounds[1].desc.stream);
    EXPECT_EQ(fixture.assets.getSound(shot), (aylo::contract::audiodevice::SoundId{.value = 1}));
    EXPECT_EQ(fixture.assets.getSound(music), (aylo::contract::audiodevice::SoundId{.value = 2}));
    EXPECT_TRUE(fixture.assets.getQueued().empty());
}

TEST(AudioAssets, LoadsASoundGivenInMemoryAndFreesItsBytes) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load(std::string{"embedded/beep.wav"}, kBytes);
    EXPECT_EQ(fixture.assets.get(id)->encoded.size(), kBytes.size());
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    ASSERT_EQ(fixture.state->sounds.size(), 1U);
    EXPECT_TRUE(fixture.state->sounds[0].path.empty());
    EXPECT_EQ(fixture.state->sounds[0].byteCount, kBytes.size());
    EXPECT_EQ(fixture.assets.get(id)->state, aylo::plugins::audio::SoundState::kLoaded);
    EXPECT_TRUE(fixture.assets.get(id)->encoded.empty());
}

TEST(AudioAssets, MarksASoundThatCannotLoadAsFailed) {
    AudioAssetsFixture fixture;
    fixture.state->failingPaths.insert("missing.wav");
    const aylo::core::asset::AssetId missing = fixture.assets.load("missing.wav");
    const aylo::core::asset::AssetId shot = fixture.assets.load("shot.wav");
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    EXPECT_EQ(fixture.assets.get(missing)->state, aylo::plugins::audio::SoundState::kFailed);
    EXPECT_FALSE(fixture.assets.get(missing)->error.empty());
    EXPECT_FALSE(fixture.assets.getSound(missing));
    EXPECT_TRUE(fixture.assets.getSound(shot));
}

TEST(AudioAssets, DestroysAnUnloadedSoundAtTheNextUpdate) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load("shot.wav");
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    const aylo::contract::audiodevice::SoundId sound = *fixture.assets.getSound(id);
    EXPECT_TRUE(fixture.assets.unload(id));
    EXPECT_EQ(fixture.assets.get(id), nullptr);
    EXPECT_TRUE(fixture.state->destroyed.empty());
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    EXPECT_EQ(fixture.state->destroyed, std::vector<aylo::contract::audiodevice::SoundId>{sound});
}

TEST(AudioAssets, ForgetsAQueuedSoundWithoutLoadingIt) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id = fixture.assets.load("shot.wav");
    EXPECT_TRUE(fixture.assets.unload(id));
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    EXPECT_TRUE(fixture.state->sounds.empty());
    EXPECT_TRUE(fixture.state->destroyed.empty());
}

TEST(AudioAssets, IgnoresAnUnknownId) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId unknown = aylo::core::asset::AssetId::fromPath("unknown.wav");
    EXPECT_FALSE(fixture.assets.unload(unknown));
    fixture.assets.setLoaded(unknown, aylo::contract::audiodevice::SoundId{.value = 7});
    fixture.assets.setFailed(unknown, "no reason");
    EXPECT_EQ(fixture.assets.get(unknown), nullptr);
}

TEST(AudioAssets, LoadsANameAndAPathAsTheSameSound) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId fromPath = fixture.assets.load("sounds/beep.wav");
    const aylo::core::asset::AssetId fromName = fixture.assets.load(std::string{"sounds/beep.wav"}, kBytes);
    EXPECT_EQ(fromPath, fromName);
    EXPECT_EQ(fixture.assets.getCount(), 1U);
}

namespace {
/// @brief A folder of sound files, removed when the test ends.
class SoundFolder {
  public:
    SoundFolder() : _path{std::filesystem::temp_directory_path() / "aylo-audio-assets-probe"} {
        std::filesystem::create_directories(_path);
    }
    ~SoundFolder() { std::filesystem::remove_all(_path); }
    SoundFolder(const SoundFolder& other) = delete;
    SoundFolder& operator=(const SoundFolder& other) = delete;
    SoundFolder(SoundFolder&& other) = delete;
    SoundFolder& operator=(SoundFolder&& other) = delete;

    /// @brief Writes @p bytes as the file @p name of the folder.
    void write(const char* name, const std::vector<std::byte>& bytes) const {
        std::ofstream file{_path / name, std::ios::binary};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    [[nodiscard]] const std::filesystem::path& getPath() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< The folder.
};
}  // namespace

TEST(AudioAssets, ProbesASoundFileBeforeTheDeviceLoadsIt) {
    AudioAssetsFixture fixture;
    const SoundFolder folder;
    folder.write("kick.wav", aylo::test::plugins::pcmWav(4410, 2, 22050));
    fixture.assets.setDirectory(folder.getPath());
    const aylo::core::asset::AssetId kick = fixture.assets.load("kick.wav");

    const std::optional<aylo::plugins::audio::SoundProbe> probe = fixture.assets.probe(kick);
    ASSERT_TRUE(probe && probe->info) << (probe ? probe->error : "no probe");
    EXPECT_EQ(*probe->info, (aylo::plugins::audio::SoundInfo{.sampleRate = 22050, .channels = 2, .frames = 4410}));
    EXPECT_EQ(fixture.assets.get(kick)->state, aylo::plugins::audio::SoundState::kQueued);

    // Kept: the file is not read again.
    std::filesystem::remove(folder.getPath() / "kick.wav");
    EXPECT_TRUE(fixture.assets.probe(kick)->info);
}

TEST(AudioAssets, SaysWhyASoundHasNoInfo) {
    AudioAssetsFixture fixture;
    const SoundFolder folder;
    folder.write("notes.wav", std::vector<std::byte>(64, std::byte{0x20}));
    fixture.assets.setDirectory(folder.getPath());

    const aylo::core::asset::AssetId missing = fixture.assets.load("music/missing.wav");
    EXPECT_EQ(fixture.assets.probe(missing)->error, "file not found: music/missing.wav");
    const aylo::core::asset::AssetId notes = fixture.assets.load("notes.wav");
    EXPECT_EQ(fixture.assets.probe(notes)->error,
              "notes.wav: not a sound file the engine decodes (WAV, OGG Vorbis, MP3, FLAC)");
    EXPECT_FALSE(fixture.assets.probe(aylo::core::asset::AssetId::fromPath("never.wav")));
}

TEST(AudioAssets, GivesTheDevicesReasonOnceItFailedToLoadASound) {
    AudioAssetsFixture fixture;
    const SoundFolder folder;
    folder.write("broken.wav", aylo::test::plugins::pcmWav(10));
    fixture.assets.setDirectory(folder.getPath());
    const aylo::core::asset::AssetId broken = fixture.assets.load("broken.wav");
    fixture.assets.setFailed(broken, "Sound 'broken.wav': cannot decode it");

    const std::optional<aylo::plugins::audio::SoundProbe> probe = fixture.assets.probe(broken);
    ASSERT_TRUE(probe);
    EXPECT_FALSE(probe->info);
    EXPECT_EQ(probe->error, "Sound 'broken.wav': cannot decode it");
}

TEST(AudioAssets, ProbesASoundGivenInMemoryAfterItsBytesAreFreed) {
    AudioAssetsFixture fixture;
    const aylo::core::asset::AssetId id =
        fixture.assets.load(std::string{"embedded/kick.wav"}, aylo::test::plugins::pcmWav(300));
    aylo::plugins::audio::updateSounds(fixture.assets, fixture.device);
    EXPECT_TRUE(fixture.assets.get(id)->encoded.empty());

    const std::optional<aylo::plugins::audio::SoundProbe> probe = fixture.assets.probe(id);
    ASSERT_TRUE(probe && probe->info);
    EXPECT_EQ(probe->info->frames, 300U);
}
