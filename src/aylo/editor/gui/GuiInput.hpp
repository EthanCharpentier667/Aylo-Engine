/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GuiInput
*/

#pragma once

#include <cstdint>

#include "GameViewport.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "editor/Export.hpp"

struct ImGuiContext;

namespace aylo::editor::gui {

/// @brief Feeds Dear ImGui with the window's events and decides which of them the game also sees: what stands between
/// the window and the game while the game is shown in a panel.
///
/// @details It is the function of the graphics plugin's PlatformEventFilter (filter()). Every event goes to ImGui. The
/// game gets the mouse only while the mouse is over the picture of the game, or a button pressed there is still down,
/// with its position placed in the scene's pixels; it gets the keyboard only while the Game panel has the focus and no
/// text field of the editor wants the keys. A Resized event is replaced by the size of the scene, which is the window
/// the game sees. The game is told nothing else it should not know.
class AYLO_EDITOR_API GuiInput {
  public:
    /// @param context The ImGui context the events go to; it must outlive the bridge.
    explicit GuiInput(ImGuiContext* context) noexcept;
    ~GuiInput() = default;
    GuiInput(const GuiInput& other) = delete;
    GuiInput& operator=(const GuiInput& other) = delete;
    GuiInput(GuiInput&& other) = delete;
    GuiInput& operator=(GuiInput&& other) = delete;

    /// @return Where the game is on the screen.
    [[nodiscard]] const GameViewport& getViewport() const noexcept { return _viewport; }

    /// @brief Tells the bridge where the game is: the Game panel calls it each frame.
    void setViewport(const GameViewport& viewport) noexcept { _viewport = viewport; }

    /// @brief Tells the bridge whether the game holds the cursor captured: the editor calls it each frame. While it
    /// does, the mouse goes to the game only (ImGui's cursor stays where it was), and Escape frees it
    /// (isCursorFreed()).
    void setCursorCaptured(bool captured) noexcept { _cursorCaptured = captured; }

    /// @return Whether Escape freed the cursor since the last click on the picture of the game: the editor then stops
    /// lending it to the game, so the user gets it back even when the game never releases it.
    [[nodiscard]] bool isCursorFreed() const noexcept { return _cursorFreed; }

    /// @brief Forgets that Escape freed the cursor (when the game stops).
    void resetCursorFreed() noexcept { _cursorFreed = false; }

    /// @return Whether the editor lends the cursor to the game: while the game plays (not paused), its Game panel has
    /// the focus, and Escape did not free it.
    [[nodiscard]] static bool lendsCursor(bool playing, bool gameFocused, bool freed) noexcept {
        return playing && gameFocused && !freed;
    }

    /// @brief Hands @p event to ImGui, then to the game if it concerns it.
    /// @param event The event of the window; changed to what the game sees.
    /// @return Whether the game sees @p event.
    [[nodiscard]] bool filter(contract::platform::Event& event);

  private:
    ImGuiContext* _context;         ///< Where the events go.
    GameViewport _viewport;         ///< Where the game is on the screen.
    std::uint32_t _gameButtons{0};  ///< Bit per mouse button pressed over the game and not released yet.
    bool _cursorCaptured{false};    ///< Whether the game holds the cursor captured.
    bool _cursorFreed{false};       ///< Whether Escape freed it since the last click on the game.
};

}  // namespace aylo::editor::gui
