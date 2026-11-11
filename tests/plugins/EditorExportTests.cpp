/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorExportTests
*/

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "editor/shipping/GameExport.hpp"

using aylo::editor::shipping::ExportChoice;
using aylo::editor::shipping::ExportResult;
using aylo::editor::shipping::runExport;

TEST(EditorExport, GivesTheChoiceToTheExporterAndItsResultBack) {
    ExportChoice seen;
    const ExportResult result = runExport(
        [&seen](const ExportChoice& choice) {
            seen = choice;
            return ExportResult{.ok = true, .folder = "exports/Duel", .bytes = 42, .libraries = {"a: every game"}};
        },
        ExportChoice{.output = "exports", .name = "Duel", .pack = false});

    EXPECT_EQ(seen.output, "exports");
    EXPECT_EQ(seen.name, "Duel");
    EXPECT_FALSE(seen.pack);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.folder, "exports/Duel");
    EXPECT_EQ(result.libraries.size(), 1U);
}

TEST(EditorExport, TurnsAnExceptionIntoAFailedResult) {
    const ExportResult result = runExport(
        [](const ExportChoice& /*choice*/) -> ExportResult {
            throw std::runtime_error{"Invalid setting 'plugins' (client): unknown plugin 'game.Bos'"};
        },
        ExportChoice{.output = "exports"});

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "Invalid setting 'plugins' (client): unknown plugin 'game.Bos'");
}

TEST(EditorExport, FailsWithoutAnExporterOrAnOutputFolder) {
    EXPECT_FALSE(runExport({}, ExportChoice{.output = "exports"}).ok);
    bool called = false;
    const ExportResult result = runExport(
        [&called](const ExportChoice& /*choice*/) {
            called = true;
            return ExportResult{.ok = true};
        },
        ExportChoice{});

    EXPECT_FALSE(called);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("folder"), std::string::npos);
}
