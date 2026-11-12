/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameExport
*/

#include "editor/shipping/GameExport.hpp"

#include <exception>
#include <string>

namespace aylo::editor::shipping {
ExportResult runExport(const Exporter& exporter, const ExportChoice& choice) {
    if (!exporter) {
        return ExportResult{.error = "This program cannot export a game"};
    }
    if (choice.output.empty()) {
        return ExportResult{.error = "Choose the folder the export goes in"};
    }
    try {
        return exporter(choice);
    } catch (const std::exception& error) {
        return ExportResult{.error = error.what()};
    } catch (...) {
        return ExportResult{.error = "The export failed"};
    }
}
}  // namespace aylo::editor::shipping
