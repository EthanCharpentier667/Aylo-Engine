/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AtlasFile
*/

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "plugins/sprite/Export.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"

namespace aylo::plugins::sprite::detail {

/// @brief Reads a file of the assets, by its path from the assets' directory: the export a `.atlas` file's `path`
/// names.
using ReadAsset = std::function<std::string(const std::filesystem::path& path)>;

/// @brief The extension of the engine's own atlas files.
inline constexpr std::string_view kAtlasFileExtension = ".atlas";

/// @brief Builds an atlas from a `.atlas` file: the fields of `engine.assets.atlas`, as a JSON object.
///
/// @details The object holds either `texture` (an image) or `path` (an Aseprite or TexturePacker export,
/// parseAtlasJson()), both relative to the file, and:
/// - `linear`: smooth sampling instead of sharp pixels (the caller's @p desc when absent);
/// - `grid` (with `texture`): `frameSize` [w, h] and `count` (1 or more) are required; `spacing` [x, y], `columns`
///   and `offset` [x, y] are optional, as SpriteAtlas::grid() takes them;
/// - `frames` (with `texture`): a list of `{ "position": [x, y], "size": [w, h] }`, with an optional `name` (its index
///   when absent, as grid() names them) and `origin` [x, y] (its centre when absent), added after the grid's;
/// - `animations`: an object of name to `{ "frames": [0, 1, 2] }` (any frame indices) or `{ "first", "count" }`
///   (count 0 or absent: up to the last frame), with `frameDuration` (seconds, 0.1), `direction` ("forward",
///   "reverse", "pingPong") and `plays` (0: forever). With `path`, they are added to the export's.
/// A key the format does not have is an error, as a misspelt field of the Luau form is.
/// @param text The file's content.
/// @param path Where it was read from, in the assets: names the atlas in errors and resolves `texture` and `path`.
/// @param read Reads the export `path` names.
/// @param desc How the image is sampled when the file gives no `linear`.
/// @throws exceptions::AtlasException If the text is not JSON, a field is missing, unknown or mistyped, or the export
/// cannot be read.
[[nodiscard]] AYLO_PLUGIN_SPRITE_API SpriteAtlas parseAtlasFile(std::string_view text,
                                                                const std::filesystem::path& path,
                                                                const ReadAsset& read,
                                                                const contract::renderer::TextureDesc& desc = {});

}  // namespace aylo::plugins::sprite::detail
