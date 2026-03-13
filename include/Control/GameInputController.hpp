#pragma once

#include <borealis.hpp>
#include <chrono>
#include <functional>
#include <vector>

namespace beiklive {

/**
 * GameInputController – raw gamepad input registration system.
 *
 * Bypasses the borealis navigation/event system entirely and operates
 * directly on the raw ControllerState snapshot captured each frame.
 *
 * Features:
 *  - Registration: registerAction(buttons, callback) binds a set of
 *    controller buttons to a callback function.
 *  - Event types: Press (rising edge), Release (falling edge),
 *    LongPress (held for >= LONG_PRESS_MS milliseconds).
 *  - Combo support: if the registered buttons array contains two or
 *    more entries, the action fires only when ALL listed buttons are
 *    simultaneously pressed.
 *  - update() must be called once per game-loop iteration with the
 *    current ControllerState.  It fires callbacks synchronously.
 *
 * Thread safety: NOT thread-safe. update() and registerAction() must
 * be called from the same thread (typically the game thread for update
 * and the main thread for registerAction during initialization, which
 * must complete before the game thread starts).
 */
class GameInputController
{
public:
    enum class KeyEvent { Press, Release, LongPress };
    using Callback = std::function<void(KeyEvent)>;

    /// Hold duration required to trigger a LongPress event (milliseconds).
    static constexpr int LONG_PRESS_MS = 500;

    /**
     * Register an action for one or more controller buttons.
     *
     * @param buttons  Array of brls::ControllerButton values (cast to int).
     *                 All buttons must be simultaneously pressed to trigger
     *                 the action (combo detection).
     * @param callback Function called with Press, Release, or LongPress.
     *
     * Single-button example:
     *   registerAction({brls::BUTTON_X}, cb);
     *
     * Combo example (L + R together):
     *   registerAction({brls::BUTTON_LB, brls::BUTTON_RB}, cb);
     */
    void registerAction(std::vector<int> buttons, Callback callback);

    /**
     * Update key states and fire registered callbacks.
     *
     * Call once per game-loop frame with the current controller snapshot.
     * For each registered action:
     *   - If ALL buttons are newly pressed → fires Press callback.
     *   - If all buttons are held for >= LONG_PRESS_MS → fires LongPress
     *     callback (fires once per press session).
     *   - If any button is released while the action was active → fires
     *     Release callback.
     */
    void update(const brls::ControllerState& state);

    /// Remove all registered actions.
    void clear();

private:
    struct Action
    {
        std::vector<int> buttons;
        Callback         callback;
        bool             active        = false; ///< true while all buttons pressed
        bool             longPressFired = false; ///< LongPress already sent this press
        std::chrono::steady_clock::time_point pressTime; ///< when Press was fired
    };

    std::vector<Action> m_actions;
};

} // namespace beiklive
