#pragma once

#ifndef HC_HEADLESS

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>

// Type definitions

// Most, if not all, of the types below are adapted from the glfw3.h header of the GLFW library
// (https://github.com/glfw/glfw) 3.4 release.

/**
 * A window button input action.
 */
enum HCButtonAction {
    Release, //!< Stop pressing a button.
    Press, //!< Begin pressing of a button.
    Repeat, //!< The repeated input which happens when a button is being held down.
};

/**
 * The identifier of a mouse button.
 */
enum HCMouseButton {
    Button1, //!< Mouse button 1, also known as the left mouse button.
    Button2, //!< Mouse button 2, also known as the right mouse button.
    Button3, //!< Mouse button 3, also known as the middle mouse button.
    Button4, //!< Mouse button 4.
    Button5, //!< Mouse button 5.
    Button6, //!< Mouse button 6.
    Button7, //!< Mouse button 7.
    Button8, //!< Mouse button 8.
};

/**
 * If this bit is set one or more Shift keys were held down.
 */
const int HC_MOD_SHIFT = 0x0001;

/**
 * If this bit is set one or more Control keys were held down.
 */
const int HC_MOD_CONTROL = 0x0002;

/**
 * If this bit is set one or more Alt keys were held down.
 */
const int HC_MOD_ALT = 0x0004;

/**
 * If this bit is set one or more Super keys were held down.
 */
const int HC_MOD_SUPER = 0x0008;

/**
 * If this bit is set the Caps Lock key is enabled.
 */
const int HC_MOD_CAPS_LOCK = 0x0010;

/**
 * If this bit is set the Num Lock key is enabled.
 */
const int HC_MOD_NUM_LOCK = 0x0020;

/**
 * Keyboard key identifiers.
 *
 * The US keyboard layout is used.
 */
enum HCKeyboardKey {
    Space = 32, //!< Space key.
    Apostrophe = 39, //!< Apostrophe key. (')
    Comma = 44, //!< Comma key. (,)
    Minus = 45, //!< Minus key. (-)
    Period = 46, //!< Period key. (.)
    Slash = 47, //!< Slash key. (/)
    Num0 = 48, //!< Number 0 key.
    Num1 = 49, //!< Number 1 key.
    Num2 = 50, //!< Number 2 key.
    Num3 = 51, //!< Number 3 key.
    Num4 = 52, //!< Number 4 key.
    Num5 = 53, //!< Number 5 key.
    Num6 = 54, //!< Number 6 key.
    Num7 = 55, //!< Number 7 key.
    Num8 = 56, //!< Number 8 key.
    Num9 = 57, //!< Number 9 key.
    Semicolon = 59, //!< Semicolon key. (;)
    Equal = 61, //!< Equal key. (=)
    A = 65, //!< Letter A key.
    B = 66, //!< Letter B key.
    C = 67, //!< Letter C key.
    D = 68, //!< Letter D key.
    E = 69, //!< Letter E key.
    F = 70, //!< Letter F key.
    G = 71, //!< Letter G key.
    H = 72, //!< Letter H key.
    I = 73, //!< Letter I key.
    J = 74, //!< Letter J key.
    K = 75, //!< Letter K key.
    L = 76, //!< Letter L key.
    M = 77, //!< Letter M key.
    N = 78, //!< Letter N key.
    O = 79, //!< Letter O key.
    P = 80, //!< Letter P key.
    Q = 81, //!< Letter Q key.
    R = 82, //!< Letter R key.
    S = 83, //!< Letter S key.
    T = 84, //!< Letter T key.
    U = 85, //!< Letter U key.
    V = 86, //!< Letter V key.
    W = 87, //!< Letter W key.
    X = 88, //!< Letter X key.
    Y = 89, //!< Letter Y key.
    Z = 90, //!< Letter Z key.
    LeftBracket = 91, //!< Left bracket key. ([)
    Backslash = 92, //!< Backslash key. (\)
    RightBracket = 93, //!< Right bracket key. (])
    GraveAccent = 96, //!< Grave accent key. (`)
    World1 = 161, //!< World key 1. (not present in US keyboard layouts)
    World2 = 162, //!< World key 2. (not present in US keyboard layouts)
    Escape = 256, //!< Escape key.
    Enter = 257, //!< Enter key.
    Tab = 258, //!< Tab key.
    Backspace = 259, //!< Backspace key.
    Insert = 260, //!< Insert key.
    Delete = 261, //!< Delete key.
    Right = 262, //!< Right arrow key.
    Left = 263, //!< Left arrow key.
    Down = 264, //!< Down arrow key.
    Up = 265, //!< Up arrow key.
    PageUp = 266, //!< Page up key.
    PageDown = 267, //!< Page down key.
    Home = 268, //!< Home key.
    End = 269, //!< End key.
    CapsLock = 280, //!< Caps lock key.
    ScrollLock = 281, //!< Scroll lock key.
    NumLock = 282, //!< Num lock key.
    PrintScreen = 283, //!< Print screen key.
    Pause = 284, //!< Pause key.
    F1 = 290, //!< Function 1 key. (F1)
    F2 = 291, //!< Function 2 key. (F2)
    F3 = 292, //!< Function 3 key. (F3)
    F4 = 293, //!< Function 4 key. (F4)
    F5 = 294, //!< Function 5 key. (F5)
    F6 = 295, //!< Function 6 key. (F6)
    F7 = 296, //!< Function 7 key. (F7)
    F8 = 297, //!< Function 8 key. (F8)
    F9 = 298, //!< Function 9 key. (F9)
    F10 = 299, //!< Function 10 key. (F10)
    F11 = 300, //!< Function 11 key. (F11)
    F12 = 301, //!< Function 12 key. (F12)
    F13 = 302, //!< Function 13 key. (F13)
    F14 = 303, //!< Function 14 key. (F14)
    F15 = 304, //!< Function 15 key. (F15)
    F16 = 305, //!< Function 16 key. (F16)
    F17 = 306, //!< Function 17 key. (F17)
    F18 = 307, //!< Function 18 key. (F18)
    F19 = 308, //!< Function 19 key. (F19)
    F20 = 309, //!< Function 20 key. (F20)
    F21 = 310, //!< Function 21 key. (F21)
    F22 = 311, //!< Function 22 key. (F22)
    F23 = 312, //!< Function 23 key. (F23)
    F24 = 313, //!< Function 24 key. (F24)
    F25 = 314, //!< Function 25 key. (F25)
    Numpad0 = 320, //!< Numpad number 0 key.
    Numpad1 = 321, //!< Numpad number 1 key.
    Numpad2 = 322, //!< Numpad number 2 key.
    Numpad3 = 323, //!< Numpad number 3 key.
    Numpad4 = 324, //!< Numpad number 4 key.
    Numpad5 = 325, //!< Numpad number 5 key.
    Numpad6 = 326, //!< Numpad number 6 key.
    Numpad7 = 327, //!< Numpad number 7 key.
    Numpad8 = 328, //!< Numpad number 8 key.
    Numpad9 = 329, //!< Numpad number 9 key.
    NumpadDecimal = 330, //!< Numpad decimal key.
    NumpadDivide = 331, //!< Numpad divide key.
    NumpadMultiply = 332, //!< Numpad multiply key.
    NumpadSubtract = 333, //!< Numpad subtract key.
    NumpadAdd = 334, //!< Numpad add key.
    NumpadEnter = 335, //!< Numpad enter key.
    NumpadEqual = 336, //!< Numpad equal key.
    LeftShift = 340, //!< Left shift key.
    LeftControl = 341, //!< Left control key.
    LeftAlt = 342, //!< Left alt key.
    LeftSuper = 343, //!< Left super key.
    RightShift = 344, //!< Right shift key.
    RightControl = 345, //!< Right control key.
    RightAlt = 346, //!< Right alt key.
    RightSuper = 347, //!< Right super key.
    Menu = 348, //!< Menu key.
};

/**
 * Events related to devices external to the system.
 */
enum HCDeviceEvent {
    Connected, //!< A device as been connected to the system.
    Disconnected, //!< A device has been disconnected from the system.
};

// Callback definitions

/**
 * @brief The type/signature of window position callback functions.
 *
 * This kind of function is typically called when a window is moved.
 *
 * @param window - The window identifier.
 * @param x - The new x-coordinate, in screen coordinates, of the upper-left corner of the content area of the window.
 * @param y - The new y-coordinate, in screen coordinates, of the upper-left corner of the content area of the window.
 */
typedef void (*HCWindowPositionCallback)(size_t window, int x, int y);

/**
 * @brief The type/signature of window size callback functions.
 *
 * This kind of function is typically called when a window is resized.
 *
 * @param window - The window identifier.
 * @param width - The new width, in screen coordinates, of the window.
 * @param height - The new height, in screen coordinates, of the window.
 */
typedef void (*HCWindowSizeCallback)(size_t window, int width, int height);

/**
 * @brief The type/signature of window close callback functions.
 *
 * This kind of function is typically called, for example, when a user clicks the window's close button.
 *
 * @param window - The window identifier.
 */
typedef void (*HCWindowCloseCallback)(size_t window);

/**
 * @brief The type/signature of window refresh callback functions.
 *
 * This kind of function is typically called when the contents of the window need to be refreshed after getting damaged.
 *
 * @param window - The window identifier.
 */
typedef void (*HCWindowRefreshCallback)(size_t window);

/**
 * @brief The type/signature of window focus callback functions.
 *
 * This kind of function is typically called when the window comes into or out of focus.
 *
 * @param window - The window identifier.
 * @param focused - `true` if the window was given input focus, or `false` if it lost it.
 */
typedef void (*HCWindowFocusCallback)(size_t window, bool focused);

/**
 * @brief The type/signature of window minimize/iconify callback functions.
 *
 * This kind of function is typically called when the window is minimized/iconified or restored.
 *
 * @param window - The window identifier.
 * @param minimised -  `true` if the window was minimized, or `false` if it was restored.
 */
typedef void (*HCWindowMinimizeCallback)(size_t window, bool minimized);

/**
 * @brief The type/signature of window maximize callback functions.
 *
 * This kind of function is typically called when the window is maximized or restored.
 *
 * @param window - The window identifier.
 * @param minimised -  `true` if the window was maximized, or `false` if it was restored.
 */
typedef void (*HCWindowMaximizeCallback)(size_t window, bool maximized);

/**
 * @brief The type/signature of window framebuffer callback functions.
 *
 * This kind of function is typically called when a window is resized.
 *
 * This differs from `HCWindowSizeCallback` in some systems where something like DPI scaling is used, and always gives
 * appropriate dimensions for rendering.
 *
 * @param window - The window identifier.
 * @param width - The new width, in pixels, of the framebuffer.
 * @param height - The new height, in pixels, of the framebuffer.
 */
typedef void (*HCWindowFramebufferCallback)(size_t window, int width, int height);

/**
 * @brief The type/signature of window scale callback functions.
 *
 * This kind of function is typically called when a window is rescaled.
 *
 * @param window - The window identifier.
 * @param x_scale - The new x-axis content scale of the window.
 * @param y_scale - The new y-axis content scale of the window.
 */
typedef void (*HCWindowScaleCallback)(size_t window, float x_scale, float y_scale);

/**
 * @brief The type/signature of mouse button callback functions.
 *
 * This kind of function is typically called when a mouse button is pressed or released.
 *
 * @param window - The window identifier.
 * @param button - The mouse button that was pressed or released.
 * @param action - Either `HCButtonAction::Press` or `HCButtonAction::Release`. Future releases may add more actions.
 * @param mods - Bit field describing which modifiers are active.
 */
typedef void (*HCWindowMouseButtonCallback)(size_t window, enum HCMouseButton button, enum HCButtonAction action,
                                            int mods);

/**
 * @brief The type/signature of cursor position callback functions.
 *
 * This kind of function is typically called when the mouse cursor is moved.
 *
 * @param window - The window identifier.
 * @param x - The new cursor x-coordinate, relative to the left edge of the content area.
 * @param y - The new cursor y-coordinate, relative to the top edge of the content area.
 */
typedef void (*HCWindowCursorPositionCallback)(size_t window, double x, double y);

/**
 * @brief The type/signature of cursor enter/leave callback functions.
 *
 * This kind of function is typically called when the mouse cursor enters or leaves the content area.
 *
 * @param window - The window identifier.
 * @param entered -  `true` if the cursor entered the window's content area, or `false` if it left it.
 */
typedef void (*HCWindowCursorEnterCallback)(size_t window, bool entered);

/**
 * @brief The type/signature of scroll callback functions.
 *
 * This kind of function is typically called when a user, for example, uses the scroll wheel.
 *
 * @param window - The window identifier.
 * @param x_offset - The scroll offset along the x-axis.
 * @param y_offset - The scroll offset along the y-axis.
 */
typedef void (*HCWindowScrollCallback)(size_t window, double x_offset, double y_offset);

/**
 * @brief The type/signature of keyboard key callback functions.
 *
 * This kind of function is typically called when a keyboard key is pressed, released or held (repeated).
 *
 * @param window - The window identifier.
 * @param key - The keyboard key that was pressed or released.
 * @param scan_code - The platform-specific scancode of the key.
 * @param action - `HCButtonAction::Press`, `HCButtonAction::Release` or `HCButtonAction::Repeat`. Future releases may
 * add more actions.
 * @param mods - Bit field describing which modifiers are active.
 */
typedef void (*HCWindowKeyCallback)(size_t window, enum HCKeyboardKey key, int scan_code, enum HCButtonAction action,
                                    int mods);

/**
 * @brief The type/signature of Unicode character callback functions.
 *
 * This kind of function is typically called when a character key is pressed, released or held (repeated), essentially,
 * when something is typed.
 *
 * @param window - The window identifier.
 * @param code_point - The Unicode code point of the character.
 */
typedef void (*HCWindowCharCallback)(size_t window, unsigned int code_point);

/**
 * @brief The type/signature of Unicode character with modifiers callback functions.
 *
 * This kind of function is typically called when a character key is pressed, released or held (repeated), essentially,
 * when something is typed.
 *
 * This is called for each input character, regardless of what modifier keys are held down.
 *
 * @param window - The window identifier.
 * @param code_point - The Unicode code point of the character.
 * @param mods - Bit field describing which modifiers are active.
 */
typedef void (*HCWindowCharModsCallback)(size_t window, unsigned int code_point, int mods);

/**
 * @brief The type/signature of path drop callback functions.
 *
 * This kind of function is typically called when a selection is dropped inside the content area.
 *
 * @param window - The window identifier.
 * @param path_count - The number of dropped paths.
 * @param paths - The UTF-8 encoded file and/or directory path names.
 */
typedef void (*HCWindowDropCallback)(size_t window, int path_count, const char *paths[]);

/**
 * @brief The type/signature of monitor configuration callback functions.
 *
 * This kind of function is typically called when a monitor is connected or disconnected
 *
 * @param window - The window identifier.
 * @param event - One of `HCDeviceEvent::Connected` or `HCDeviceEvent::Disconnected`. Future releases may add more events.
 */
typedef void (*HCWindowMonitorCallback)(size_t window, enum HCDeviceEvent event);

/**
 * @brief The type/signature of joystick configuration callback functions.
 *
 * This kind of function is typically called when a joystick is connected or disconnected
 *
 * @param window - The window identifier.
 * @param event - One of `HCDeviceEvent::Connected` or `HCDeviceEvent::Disconnected`. Future releases may add more events.
 */
typedef void (*HCWindowJoystickCallback)(size_t window, enum HCDeviceEvent event);

// Window type and function declarations

/**
 * @brief A high-level abstraction of a graphical window.
 */
struct HCWindow {
    void *handle; //!< The internal handle of this window.
    size_t id; //!< The global id of this window.
};

/**
 * @brief Initialisation parameters for a new window.
 */
struct HCWindowParams {
    uint32_t width; //!< The width of the window.
    uint32_t height; //!< The height of the window.
    int pos_x; //!< A hint for the initial position of the window, in the x-axis.
    int pos_y; //!< A hint for the initial position of the window, in the y-axis.
    const char *name; //!< The name of the window, UTF-8 encoded.
};

/**
 * @brief Processes all pending window events.
 *
 * This function is meant to be called in a loop, once per frame, it MUST be called in non-headless configurations and
 * it MUST be from the main thread.
 */
void hc_poll_events();

/**
 * @brief Constructs a new `HCWindow`.
 *
 * @param params the window initialisation parameters.
 *
 * @return The newly created `HCWindow`. The returned object is invalid if some error has occurred.
 */
struct HCWindow hc_new_window(struct HCWindowParams params);

/**
 * @brief Destroys a `HCWindow`.
 *
 * The window will become unusable immediately, but will only truly be destroyed once its resources are no longer
 * being used, which should be after after a few frames, depending on the number of frames in flight.
 *
 * @param window a pointer to the `HCWindow` to be destroyed
 */
void hc_destroy_window(struct HCWindow *window);

/**
 * @brief Sets the window position callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_position_callback(struct HCWindow *window, HCWindowPositionCallback callback);

/**
 * @brief Sets the window size callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_size_callback(struct HCWindow *window, HCWindowSizeCallback callback);

/**
 * @brief Sets the window close callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_close_callback(struct HCWindow *window, HCWindowCloseCallback callback);

/**
 * @brief Sets the window refresh callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_refresh_callback(struct HCWindow *window, HCWindowRefreshCallback callback);

/**
 * @brief Sets the window focus callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_focus_callback(struct HCWindow *window, HCWindowFocusCallback callback);

/**
 * @brief Sets the window minimize callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_minimize_callback(struct HCWindow *window, HCWindowMinimizeCallback minimized);

/**
 * @brief Sets the window maximize callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_maximize_callback(struct HCWindow *window, HCWindowMaximizeCallback maximized);

/**
 * @brief Sets the window framebuffer callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_framebuffer_callback(struct HCWindow *window, HCWindowFramebufferCallback callback);

/**
 * @brief Sets the window scale callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_scale_callback(struct HCWindow *window, HCWindowScaleCallback callback);

/**
 * @brief Sets the mouse button callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_mouse_button_callback(struct HCWindow *window, HCWindowMouseButtonCallback callback);

/**
 * @brief Sets the cursor position callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_cursor_position_callback(struct HCWindow *window, HCWindowCursorPositionCallback callback);

/**
 * @brief Sets the cursor enter callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_cursor_enter_callback(struct HCWindow *window, HCWindowCursorEnterCallback callback);

/**
 * @brief Sets the scroll callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_scroll_callback(struct HCWindow *window, HCWindowScrollCallback callback);

/**
 * @brief Sets the key callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_key_callback(struct HCWindow *window, HCWindowKeyCallback callback);

/**
 * @brief Sets the character callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_char_callback(struct HCWindow *window, HCWindowCharCallback callback);

/**
 * @brief Sets the character with modifiers callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_char_mods_callback(struct HCWindow *window, HCWindowCharModsCallback callback);

/**
 * @brief Sets the window drop callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_drop_callback(struct HCWindow *window, HCWindowDropCallback callback);

//void hc_set_window_monitor_callback(struct HCWindow *window, HCWindowMonitorCallback callback);
//
//void hc_set_window_joystick_callback(struct HCWindow *window, HCWindowJoystickCallback callback);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HC_HEADLESS

