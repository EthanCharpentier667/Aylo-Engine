/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AtlasFile
*/

#include "plugins/sprite/detail/AtlasFile.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/math/Rect.hpp"
#include "core/asset/AssetFiles.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/detail/AtlasJson.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

namespace aylo::plugins::sprite::detail {

namespace {
/// @brief The parsed file. Ordered: the animations keep the file's order, as its errors do.
using Json = nlohmann::ordered_json;

/// @brief Seconds a frame of an animation lasts when the file does not say, as for `engine.assets.atlas`.
constexpr float kDefaultFrameDuration = 0.1F;

/// @brief Throws if @p object, named @p what in the error, has a key that is not one of @p keys.
void checkKeys(const Json& object, const std::string_view what, const std::initializer_list<std::string_view> keys) {
    if (!object.is_object()) {
        throw exceptions::AtlasException{std::format("{} must be an object", what)};
    }
    for (const auto& [key, value] : object.items()) {
        bool known = false;
        for (const std::string_view expected : keys) {
            known = known || key == expected;
        }
        if (!known) {
            std::string list;
            for (const std::string_view expected : keys) {
                list += std::format("{}'{}'", list.empty() ? "" : ", ", expected);
            }
            throw exceptions::AtlasException{std::format("{}: unknown field '{}' (expected {})", what, key, list)};
        }
    }
}

/// @return The field @p key of @p object as a [x, y] pair, or nothing when absent.
std::optional<glm::vec2> readVec2(const Json& object, const std::string_view what, const std::string& key) {
    if (!object.contains(key)) {
        return std::nullopt;
    }
    const Json& pair = object.at(key);
    if (!pair.is_array() || pair.size() != 2 || !pair.at(0).is_number() || !pair.at(1).is_number()) {
        throw exceptions::AtlasException{std::format("{}: '{}' must be two numbers, [x, y]", what, key)};
    }
    return glm::vec2{pair.at(0).get<float>(), pair.at(1).get<float>()};
}

/// @return The field @p key of @p object as a whole number of 0 or more, or nothing when absent.
std::optional<std::uint32_t> readCount(const Json& object, const std::string_view what, const std::string& key) {
    if (!object.contains(key)) {
        return std::nullopt;
    }
    const Json& value = object.at(key);
    if (!value.is_number_unsigned()) {
        throw exceptions::AtlasException{std::format("{}: '{}' must be a whole number, 0 or more", what, key)};
    }
    return value.get<std::uint32_t>();
}

/// @return The image or export @p key names, relative to the atlas file at @p path, as a path of the assets.
std::string resolve(const std::filesystem::path& path, const Json& root, const std::string& key) {
    const Json& value = root.at(key);
    if (!value.is_string() || value.get<std::string>().empty()) {
        throw exceptions::AtlasException{std::format("'{}' must be a path, relative to the atlas file", key)};
    }
    return core::asset::normalizePath(path.parent_path() / value.get<std::string>());
}

/// @return The atlas the `grid` of the file cuts from @p texture, or an empty one.
SpriteAtlas readGrid(const Json& root, std::string texture, const contract::renderer::TextureDesc& desc) {
    if (!root.contains("grid")) {
        return SpriteAtlas{std::move(texture), desc};
    }
    const Json& grid = root.at("grid");
    constexpr std::string_view kWhat = "'grid'";
    checkKeys(grid, kWhat, {"frameSize", "count", "spacing", "columns", "offset"});
    const std::optional<glm::vec2> frameSize = readVec2(grid, kWhat, "frameSize");
    const std::optional<std::uint32_t> count = readCount(grid, kWhat, "count");
    if (!frameSize || !count || *count == 0) {
        throw exceptions::AtlasException{"'grid': expected a 'frameSize' and a 'count' of 1 or more"};
    }
    return SpriteAtlas::grid(
        std::move(texture), *frameSize, *count, readVec2(grid, kWhat, "spacing").value_or(glm::vec2{0.0F}),
        readCount(grid, kWhat, "columns").value_or(0), readVec2(grid, kWhat, "offset").value_or(glm::vec2{0.0F}), desc);
}

/// @brief Adds the `frames` of the file to @p atlas, after its others.
void readFrames(const Json& root, SpriteAtlas& atlas) {
    if (!root.contains("frames")) {
        return;
    }
    const Json& frames = root.at("frames");
    if (!frames.is_array()) {
        throw exceptions::AtlasException{"'frames' must be a list"};
    }
    std::uint32_t number = 0;
    for (const Json& frame : frames) {
        ++number;
        const std::string what = std::format("frame {}", number);
        checkKeys(frame, what, {"name", "position", "size", "origin"});
        const std::optional<glm::vec2> position = readVec2(frame, what, "position");
        const std::optional<glm::vec2> size = readVec2(frame, what, "size");
        if (!position || !size) {
            throw exceptions::AtlasException{std::format("{}: expected a 'position' and a 'size'", what)};
        }
        std::string name = std::format("{}", atlas.getFrames().size());
        if (frame.contains("name")) {
            if (!frame.at("name").is_string()) {
                throw exceptions::AtlasException{std::format("{}: 'name' must be a string", what)};
            }
            name = frame.at("name").get<std::string>();
        }
        atlas.addFrame(std::move(name), contract::renderer::math::Rect{*position, *size},
                       readVec2(frame, what, "origin"));
    }
}

/// @return The direction named @p name.
AnimationDirection readDirection(const std::string_view what, const std::string& name) {
    if (name == "forward") {
        return AnimationDirection::kForward;
    }
    if (name == "reverse") {
        return AnimationDirection::kReverse;
    }
    if (name == "pingPong") {
        return AnimationDirection::kPingPong;
    }
    throw exceptions::AtlasException{
        std::format("{}: unknown direction '{}' (expected 'forward', 'reverse' or 'pingPong')", what, name)};
}

/// @return The frame indices of @p animation: its `frames`, or `first` and `count` (0 or absent: up to the last).
std::vector<std::uint32_t> readFrameIndices(const Json& animation, const std::string_view what,
                                            const SpriteAtlas& atlas) {
    std::vector<std::uint32_t> indices;
    if (animation.contains("frames")) {
        if (animation.contains("first") || animation.contains("count")) {
            throw exceptions::AtlasException{std::format("{}: expected either 'frames' or 'first' and 'count'", what)};
        }
        const Json& frames = animation.at("frames");
        if (!frames.is_array() || frames.empty()) {
            throw exceptions::AtlasException{std::format("{}: 'frames' must be a list of frame indices", what)};
        }
        for (const Json& index : frames) {
            if (!index.is_number_unsigned()) {
                throw exceptions::AtlasException{
                    std::format("{}: 'frames' must list frame indices: whole numbers, 0 or more", what)};
            }
            indices.push_back(index.get<std::uint32_t>());
        }
        return indices;
    }
    const std::uint32_t first = readCount(animation, what, "first").value_or(0);
    const auto frameCount = static_cast<std::uint32_t>(atlas.getFrames().size());
    std::uint32_t count = readCount(animation, what, "count").value_or(0);
    if (count == 0) {
        // Up to the last frame; one frame past the end when `first` is not a frame, which addAnimation() refuses.
        count = first < frameCount ? frameCount - first : 1;
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        indices.push_back(first + index);
    }
    return indices;
}

/// @brief Adds the `animations` of the file to @p atlas.
void readAnimations(const Json& root, SpriteAtlas& atlas) {
    if (!root.contains("animations")) {
        return;
    }
    const Json& animations = root.at("animations");
    if (!animations.is_object()) {
        throw exceptions::AtlasException{"'animations' must be an object of name to animation"};
    }
    for (const auto& [name, animation] : animations.items()) {
        const std::string what = std::format("animation '{}'", name);
        checkKeys(animation, what, {"frames", "first", "count", "frameDuration", "direction", "plays"});
        float frameDuration = kDefaultFrameDuration;
        if (animation.contains("frameDuration")) {
            if (!animation.at("frameDuration").is_number() || animation.at("frameDuration").get<float>() <= 0.0F) {
                throw exceptions::AtlasException{std::format("{}: 'frameDuration' must be seconds, above 0", what)};
            }
            frameDuration = animation.at("frameDuration").get<float>();
        }
        AnimationDirection direction = AnimationDirection::kForward;
        if (animation.contains("direction")) {
            if (!animation.at("direction").is_string()) {
                throw exceptions::AtlasException{std::format("{}: 'direction' must be a string", what)};
            }
            direction = readDirection(what, animation.at("direction").get<std::string>());
        }
        atlas.addAnimation(name, readFrameIndices(animation, what, atlas), frameDuration, direction,
                           readCount(animation, what, "plays").value_or(0));
    }
}

/// @return How the image is sampled: the file's `linear` when it has one, @p desc otherwise.
contract::renderer::TextureDesc readDesc(const Json& root, contract::renderer::TextureDesc desc) {
    if (root.contains("linear")) {
        if (!root.at("linear").is_boolean()) {
            throw exceptions::AtlasException{"'linear' must be true or false"};
        }
        desc.filter = root.at("linear").get<bool>() ? contract::renderer::TextureFilter::kLinear
                                                    : contract::renderer::TextureFilter::kNearest;
    }
    return desc;
}

/// @return The atlas the file at @p path describes.
SpriteAtlas readAtlas(const Json& root, const std::filesystem::path& path, const ReadAsset& read,
                      const contract::renderer::TextureDesc& fallback) {
    checkKeys(root, "the atlas file", {"path", "texture", "linear", "grid", "frames", "animations"});
    if (root.contains("path") == root.contains("texture")) {
        throw exceptions::AtlasException{
            "expected either a 'path' (an Aseprite or TexturePacker export) or a 'texture' (an image)"};
    }
    const contract::renderer::TextureDesc desc = readDesc(root, fallback);
    if (root.contains("path")) {
        if (root.contains("grid") || root.contains("frames")) {
            throw exceptions::AtlasException{"'grid' and 'frames' cut a 'texture', not an export named by 'path'"};
        }
        const std::string exported = resolve(path, root, "path");
        SpriteAtlas atlas = parseAtlasJson(read(exported), exported, desc);
        readAnimations(root, atlas);
        return atlas;
    }
    SpriteAtlas atlas = readGrid(root, resolve(path, root, "texture"), desc);
    readFrames(root, atlas);
    readAnimations(root, atlas);
    return atlas;
}
}  // namespace

SpriteAtlas parseAtlasFile(std::string_view text, const std::filesystem::path& path, const ReadAsset& read,
                           const contract::renderer::TextureDesc& desc) {
    const std::string name = path.generic_string();
    try {
        return readAtlas(Json::parse(text), path, read, desc);
    } catch (const nlohmann::json::exception& error) {
        throw exceptions::AtlasException{std::format("Atlas '{}': {}", name, error.what())};
    } catch (const exceptions::AtlasException& error) {
        throw exceptions::AtlasException{std::format("Atlas '{}': {}", name, error.what())};
    } catch (const std::exception& error) {
        throw exceptions::AtlasException{std::format("Atlas '{}': {}", name, error.what())};
    }
}

}  // namespace aylo::plugins::sprite::detail
