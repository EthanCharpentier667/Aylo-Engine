/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptResolverTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "ScriptsDirectory.hpp"
#include "aylo/scripting/ErrorKind.hpp"
#include "aylo/scripting/exceptions/ScriptLoadException.hpp"
#include "aylo/scripting/modules/ScriptResolver.hpp"
#include "core/vfs/FileSource.hpp"
#include "core/vfs/Pack.hpp"

using aylo::scripting::ErrorKind;
using aylo::scripting::exceptions::ScriptLoadException;
using aylo::scripting::modules::ScriptResolver;
using aylo::tests::ScriptsDirectory;

namespace {
/// @brief Expects the resolution to fail with @p kind and a message containing @p text.
void expectError(const aylo::scripting::Result<aylo::luau::ModuleSource>& result, const ErrorKind kind,
                 const std::string_view text) {
    ASSERT_FALSE(result) << "resolved to " << result->key;
    EXPECT_EQ(result.error().kind(), kind) << result.error().message();
    EXPECT_NE(std::string{result.error().message()}.find(text), std::string::npos) << result.error().message();
}
}  // namespace

TEST(ScriptResolver, ResolvesAFile) {
    const ScriptsDirectory scripts{"aylo-resolver-file"};
    scripts.write("config/player.luau", "return 1");
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("main.luau", "./config/player");
    ASSERT_TRUE(module) << module.error().message();
    EXPECT_EQ(module->key, "config/player.luau");
    EXPECT_EQ(module->chunkName, "scripts/config/player.luau");
    EXPECT_EQ(module->source, "return 1");
}

TEST(ScriptResolver, ResolvesAFolderWithAnInit) {
    const ScriptsDirectory scripts{"aylo-resolver-init"};
    scripts.write("ui/init.luau", "return 'ui'");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("", "@scripts/ui");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "ui/init.luau");
    EXPECT_EQ(module->chunkName, "scripts/ui/init.luau");
}

TEST(ScriptResolver, AFileAndAnInitTogetherAreAmbiguous) {
    const ScriptsDirectory scripts{"aylo-resolver-ambiguous"};
    scripts.write("a.luau", "");
    scripts.write("a/init.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/a"), ErrorKind::kAmbiguousModule, "ambiguous");
}

TEST(ScriptResolver, ANotFoundNamesBothCandidates) {
    const ScriptsDirectory scripts{"aylo-resolver-missing"};
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("main.luau", "./config/player"), ErrorKind::kModuleNotFound,
                "scripts/config/player.luau or scripts/config/player/init.luau");
}

TEST(ScriptResolver, AFolderNamedLikeAFileIsNotAModule) {
    const ScriptsDirectory scripts{"aylo-resolver-folder-file"};
    scripts.write("a.luau/x.txt", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/a"), ErrorKind::kModuleNotFound, "not found");
}

TEST(ScriptResolver, ResolvesFromTheFolderOfTheRequester) {
    const ScriptsDirectory scripts{"aylo-resolver-relative"};
    scripts.write("a/b/main.luau", "");
    scripts.write("a/b/sibling.luau", "");
    scripts.write("a/up.luau", "");
    scripts.write("top.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("a/b/main.luau", "./sibling")->key, "a/b/sibling.luau");
    EXPECT_EQ(resolver("a/b/main.luau", "../up")->key, "a/up.luau");
    EXPECT_EQ(resolver("a/b/main.luau", "../../top")->key, "top.luau");
}

TEST(ScriptResolver, ARootRequireIsRelativeToScripts) {
    const ScriptsDirectory scripts{"aylo-resolver-root-require"};
    scripts.write("server/main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("", "./server/main")->key, "server/main.luau");
}

TEST(ScriptResolver, RefusesToLeaveScripts) {
    const ScriptsDirectory scripts{"aylo-resolver-escape"};
    scripts.write("a/main.luau", "");
    scripts.write("../aylo-resolver-escape-secret.luau", "return 'secret'");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("a/main.luau", "../../aylo-resolver-escape-secret"), ErrorKind::kOutsideScripts,
                "escapes scripts/");
    expectError(resolver("", "../project"), ErrorKind::kOutsideScripts, "escapes scripts/");
    expectError(resolver("main.luau", "../x"), ErrorKind::kOutsideScripts, "escapes scripts/");
    std::filesystem::remove(scripts.path() / ".." / "aylo-resolver-escape-secret.luau");
}

TEST(ScriptResolver, RefusesTheInvalidForms) {
    const ScriptsDirectory scripts{"aylo-resolver-invalid"};
    scripts.write("a.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "a"), ErrorKind::kInvalidPath, "must start with");
    expectError(resolver("", ""), ErrorKind::kInvalidPath, "empty");
    expectError(resolver("", "./a.luau"), ErrorKind::kInvalidPath, "extension");
    expectError(resolver("", ".\\a"), ErrorKind::kInvalidPath, "separator");
    expectError(resolver("", "./a//b"), ErrorKind::kInvalidPath, "empty segment");
    expectError(resolver("", "./a/./b"), ErrorKind::kInvalidPath, "start of a path");
    expectError(resolver("", "./a/"), ErrorKind::kInvalidPath, "end with '/'");
    expectError(resolver("", "@scripts/../a"), ErrorKind::kInvalidPath, "start of a path");
}

TEST(ScriptResolver, TheScriptsAliasIsTheRoot) {
    const ScriptsDirectory scripts{"aylo-resolver-scripts-alias"};
    scripts.write("a/b/main.luau", "");
    scripts.write("shared/util.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("a/b/main.luau", "@scripts/shared/util")->key, "shared/util.luau");
    EXPECT_EQ(resolver("a/b/main.luau", "@SCRIPTS/shared/util")->key, "shared/util.luau");
}

TEST(ScriptResolver, ABareScriptsAliasIsTheRootInit) {
    const ScriptsDirectory scripts{"aylo-resolver-bare-scripts"};
    scripts.write("init.luau", "return 'root'");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("x.luau", "@scripts");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "init.luau");
    EXPECT_EQ(module->chunkName, "scripts/init.luau");

    const ScriptsDirectory empty{"aylo-resolver-bare-scripts-empty"};
    expectError(ScriptResolver{empty.path()}("", "@scripts"), ErrorKind::kModuleNotFound, "scripts/init.luau");
}

TEST(ScriptResolver, SelfIsTheFolderOfAFile) {
    const ScriptsDirectory scripts{"aylo-resolver-self-file"};
    scripts.write("config/main.luau", "");
    scripts.write("config/player.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("config/main.luau", "@self/player")->key, "config/player.luau");
}

TEST(ScriptResolver, SelfIsTheFolderOfAnInit) {
    const ScriptsDirectory scripts{"aylo-resolver-self-init"};
    scripts.write("ui/init.luau", "");
    scripts.write("ui/button.luau", "");
    scripts.write("button.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("ui/init.luau", "@self/button")->key, "ui/button.luau");
}

TEST(ScriptResolver, SelfNeedsARequester) {
    const ScriptsDirectory scripts{"aylo-resolver-self-root"};
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@self/a"), ErrorKind::kInvalidPath, "@self");
}

TEST(ScriptResolver, InAnInitDotMeansTheSiblingsOfItsFolder) {
    const ScriptsDirectory scripts{"aylo-resolver-init-siblings"};
    scripts.write("ui/init.luau", "");
    scripts.write("ui/button.luau", "");
    scripts.write("theme.luau", "");
    scripts.write("lib/util.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("ui/init.luau", "./theme")->key, "theme.luau");
    EXPECT_EQ(resolver("ui/init.luau", "./lib/util")->key, "lib/util.luau");
    EXPECT_EQ(resolver("ui/init.luau", "./ui/button")->key, "ui/button.luau");
    expectError(resolver("init.luau", "./x"), ErrorKind::kOutsideScripts, "escapes scripts/");
}

TEST(ScriptResolver, UnknownAliasListsTheKnownOnes) {
    const ScriptsDirectory scripts{"aylo-resolver-unknown-alias"};
    const ScriptResolver resolver{scripts.path(), {{"shared", "shared"}, {"Assets", "assets"}}};

    const auto result = resolver("", "@nope/a");
    expectError(result, ErrorKind::kUnknownAlias, "unknown alias '@nope'");
    EXPECT_NE(std::string{result.error().message()}.find("@assets, @scripts, @self, @shared"), std::string::npos)
        << result.error().message();
}

TEST(ScriptResolver, AliasesIgnoreCase) {
    const ScriptsDirectory scripts{"aylo-resolver-alias-case"};
    scripts.write("shared/util.luau", "");
    const ScriptResolver resolver{scripts.path(), {{"Shared", "shared"}}};

    EXPECT_EQ(resolver("", "@shared/util")->key, "shared/util.luau");
    EXPECT_EQ(resolver("", "@SHARED/util")->key, "shared/util.luau");
}

TEST(ScriptResolver, AnExtraAliasPointsToAFolder) {
    const ScriptsDirectory scripts{"aylo-resolver-extra-alias"};
    scripts.write("lib/shared/util.luau", "");
    scripts.write("lib/shared/init.luau", "");
    const ScriptResolver resolver{scripts.path(), {{"shared", "lib/shared/"}}};

    EXPECT_EQ(resolver("a/b.luau", "@shared/util")->key, "lib/shared/util.luau");
    EXPECT_EQ(resolver("a/b.luau", "@shared")->key, "lib/shared/init.luau");
}

TEST(ScriptResolver, RejectsBadAliasesAtConstruction) {
    const ScriptsDirectory scripts{"aylo-resolver-bad-alias"};
    using Aliases = std::map<std::string, std::string>;
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"a/b", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"scripts", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"Self", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"out", "../elsewhere"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"out", "a/../../elsewhere"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"abs", scripts.path().generic_string()}}}),
                 ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"empty", ""}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"a", "x"}, {"A", "y"}}}), ScriptLoadException);
}

TEST(ScriptResolver, RejectsAnAliasThroughALinkPointingOut) {
    const ScriptsDirectory scripts{"aylo-resolver-alias-link"};
    const ScriptsDirectory outside{"aylo-resolver-alias-link-outside"};
    if (!scripts.link("out", outside.path())) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    EXPECT_THROW((ScriptResolver{scripts.path(), {{"out", "out"}}}), ScriptLoadException);
}

TEST(ScriptResolver, RefusesAFileWhoseCaseDiffers) {
    const ScriptsDirectory scripts{"aylo-resolver-case-file"};
    scripts.write("Config.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/config"), ErrorKind::kCaseMismatch, "case mismatch: the file is 'Config.luau'");
    EXPECT_TRUE(resolver("", "@scripts/Config"));
}

TEST(ScriptResolver, RefusesAFolderWhoseCaseDiffers) {
    const ScriptsDirectory scripts{"aylo-resolver-case-folder"};
    scripts.write("Shared/util.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/shared/util"), ErrorKind::kCaseMismatch, "'Shared'");
    EXPECT_TRUE(resolver("", "@scripts/Shared/util"));
}

TEST(ScriptResolver, RefusesALinkedFileOutsideScripts) {
    const ScriptsDirectory scripts{"aylo-resolver-link-out"};
    const ScriptsDirectory outside{"aylo-resolver-link-out-target"};
    outside.write("secret.luau", "return 'secret'");
    if (!scripts.link("secret.luau", outside.path() / "secret.luau") || !scripts.link("dir", outside.path())) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/secret"), ErrorKind::kOutsideScripts, "escapes scripts/");
    expectError(resolver("", "@scripts/dir/secret"), ErrorKind::kOutsideScripts, "escapes scripts/");
}

TEST(ScriptResolver, FollowsALinkInsideScripts) {
    const ScriptsDirectory scripts{"aylo-resolver-link-in"};
    scripts.write("real/util.luau", "return 1");
    if (!scripts.link("alias.luau", scripts.path() / "real" / "util.luau")) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("", "@scripts/alias");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "alias.luau");
    EXPECT_EQ(module->source, "return 1");
}

TEST(ScriptResolver, ResolvesAnEntry) {
    const ScriptsDirectory scripts{"aylo-resolver-entry"};
    scripts.write("server/main.luau", "return 'server'");
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver.resolveEntry("server/main");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "server/main.luau");
    EXPECT_EQ(module->chunkName, "scripts/server/main.luau");
    EXPECT_EQ(module->source, "return 'server'");
    EXPECT_EQ(resolver.resolveEntry("main")->key, "main.luau");
}

TEST(ScriptResolver, RefusesInvalidEntries) {
    const ScriptsDirectory scripts{"aylo-resolver-bad-entry"};
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver.resolveEntry(""), ErrorKind::kInvalidPath, "empty");
    expectError(resolver.resolveEntry("./main"), ErrorKind::kInvalidPath, "no prefix");
    expectError(resolver.resolveEntry("../main"), ErrorKind::kInvalidPath, "no prefix");
    expectError(resolver.resolveEntry("@scripts/main"), ErrorKind::kInvalidPath, "no prefix");
    expectError(resolver.resolveEntry("main.luau"), ErrorKind::kInvalidPath, "extension");
    expectError(resolver.resolveEntry("a/../main"), ErrorKind::kInvalidPath, "invalid entry");
    expectError(resolver.resolveEntry("/main"), ErrorKind::kInvalidPath, "invalid entry");
    expectError(resolver.resolveEntry("main/"), ErrorKind::kInvalidPath, "invalid entry");
    expectError(resolver.resolveEntry("missing"), ErrorKind::kModuleNotFound, "not found");
}

namespace {
/// @return The scripts of a pack written to @p file, as the scripting plugin reads them from an exported game.
std::shared_ptr<const aylo::core::vfs::FileSource> packedScripts(const std::filesystem::path& file) {
    using aylo::core::vfs::PackEntry;
    aylo::core::vfs::writePack(file,
                               {
                                   PackEntry{.path = "project.luau", .data = std::string{""}},
                                   PackEntry{.path = "scripts/config/player.luau", .data = std::string{"return 1"}},
                                   PackEntry{.path = "scripts/ui/init.luau", .data = std::string{"return 2"}},
                                   PackEntry{.path = "scripts/Enemy.luau", .data = std::string{"return 3"}},
                               });
    return std::make_shared<aylo::core::vfs::SubSource>(std::make_shared<aylo::core::vfs::PackSource>(file), "scripts");
}
}  // namespace

TEST(ScriptResolver, ResolvesTheScriptsOfASource) {
    const ScriptsDirectory scripts{"aylo-resolver-source"};
    const ScriptResolver resolver{packedScripts(scripts.path() / "game.pak"), {{"cfg", "config"}}};

    const auto player = resolver("", "@scripts/config/player");
    ASSERT_TRUE(player) << player.error().message();
    EXPECT_EQ(player->key, "config/player.luau");
    EXPECT_EQ(player->chunkName, "scripts/config/player.luau");
    EXPECT_EQ(player->source, "return 1");
    const auto ui = resolver("", "@scripts/ui");
    ASSERT_TRUE(ui) << ui.error().message();
    EXPECT_EQ(ui->key, "ui/init.luau");
    const auto aliased = resolver("", "@cfg/player");
    ASSERT_TRUE(aliased) << aliased.error().message();
    EXPECT_EQ(aliased->key, "config/player.luau");
    EXPECT_TRUE(resolver.root().empty());
}

TEST(ScriptResolver, KeepsTheCaseRulesInASource) {
    const ScriptsDirectory scripts{"aylo-resolver-source-case"};
    const ScriptResolver resolver{packedScripts(scripts.path() / "game.pak")};

    expectError(resolver("", "@scripts/enemy"), ErrorKind::kCaseMismatch, "'Enemy.luau'");
    expectError(resolver("", "@scripts/Config/player"), ErrorKind::kCaseMismatch, "'config'");
    expectError(resolver("", "@scripts/missing"), ErrorKind::kModuleNotFound, "scripts/missing.luau");
    expectError(resolver("", "../project"), ErrorKind::kOutsideScripts, "escapes scripts/");
}
