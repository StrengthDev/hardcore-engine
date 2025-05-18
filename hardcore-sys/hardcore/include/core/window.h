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
 * Mouse cursor modes.
 */
enum HCCursorMode {
    HCCursorMode_Normal, //!< Cursor is visible and its motion is not limited by anything.
    HCCursorMode_Hidden, //!< Cursor becomes invisible while over the window, but its motion is not limited by anything.
    HCCursorMode_Captured, //!< Cursor is visible, but it will be confined to the window's area while it is in focus.
    /**
     * @brief When the window is in focus, the cursor is hidden and locked to the window's area.
     *
     * This mode should be ideal for things such as controlling a camera via mouse motion.
     *
     * On systems that support it, this mode will enable raw mouse motion input.
     */
    HCCursorMode_Disabled,
};

/**
 * A window button input action.
 */
enum HCButtonAction {
    HCButtonAction_Release, //!< Stop pressing a button.
    HCButtonAction_Press, //!< Begin pressing of a button.
    HCButtonAction_Repeat, //!< The repeated input which happens when a button is being held down.
};

/**
 * The identifier of a mouse button.
 */
enum HCMouseButton {
    HCMouseButton_Button1, //!< Mouse button 1, also known as the left mouse button.
    HCMouseButton_Button2, //!< Mouse button 2, also known as the right mouse button.
    HCMouseButton_Button3, //!< Mouse button 3, also known as the middle mouse button.
    HCMouseButton_Button4, //!< Mouse button 4.
    HCMouseButton_Button5, //!< Mouse button 5.
    HCMouseButton_Button6, //!< Mouse button 6.
    HCMouseButton_Button7, //!< Mouse button 7.
    HCMouseButton_Button8, //!< Mouse button 8.

    HCMouseButton_Left = HCMouseButton_Button1, //!< The left mouse button, an alias for mouse button 1.
    HCMouseButton_Right = HCMouseButton_Button2, //!< The right mouse button, an alias for mouse button 2.
    HCMouseButton_Middle = HCMouseButton_Button2, //!< The middle mouse button, an alias for mouse button 3.
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
    HCKeyboardKey_Space = 32, //!< Space key.
    HCKeyboardKey_Apostrophe = 39, //!< Apostrophe key. (')
    HCKeyboardKey_Comma = 44, //!< Comma key. (,)
    HCKeyboardKey_Minus = 45, //!< Minus key. (-)
    HCKeyboardKey_Period = 46, //!< Period key. (.)
    HCKeyboardKey_Slash = 47, //!< Slash key. (/)
    HCKeyboardKey_Num0 = 48, //!< Number 0 key.
    HCKeyboardKey_Num1 = 49, //!< Number 1 key.
    HCKeyboardKey_Num2 = 50, //!< Number 2 key.
    HCKeyboardKey_Num3 = 51, //!< Number 3 key.
    HCKeyboardKey_Num4 = 52, //!< Number 4 key.
    HCKeyboardKey_Num5 = 53, //!< Number 5 key.
    HCKeyboardKey_Num6 = 54, //!< Number 6 key.
    HCKeyboardKey_Num7 = 55, //!< Number 7 key.
    HCKeyboardKey_Num8 = 56, //!< Number 8 key.
    HCKeyboardKey_Num9 = 57, //!< Number 9 key.
    HCKeyboardKey_Semicolon = 59, //!< Semicolon key. (;)
    HCKeyboardKey_Equal = 61, //!< Equal key. (=)
    HCKeyboardKey_A = 65, //!< Letter A key.
    HCKeyboardKey_B = 66, //!< Letter B key.
    HCKeyboardKey_C = 67, //!< Letter C key.
    HCKeyboardKey_D = 68, //!< Letter D key.
    HCKeyboardKey_E = 69, //!< Letter E key.
    HCKeyboardKey_F = 70, //!< Letter F key.
    HCKeyboardKey_G = 71, //!< Letter G key.
    HCKeyboardKey_H = 72, //!< Letter H key.
    HCKeyboardKey_I = 73, //!< Letter I key.
    HCKeyboardKey_J = 74, //!< Letter J key.
    HCKeyboardKey_K = 75, //!< Letter K key.
    HCKeyboardKey_L = 76, //!< Letter L key.
    HCKeyboardKey_M = 77, //!< Letter M key.
    HCKeyboardKey_N = 78, //!< Letter N key.
    HCKeyboardKey_O = 79, //!< Letter O key.
    HCKeyboardKey_P = 80, //!< Letter P key.
    HCKeyboardKey_Q = 81, //!< Letter Q key.
    HCKeyboardKey_R = 82, //!< Letter R key.
    HCKeyboardKey_S = 83, //!< Letter S key.
    HCKeyboardKey_T = 84, //!< Letter T key.
    HCKeyboardKey_U = 85, //!< Letter U key.
    HCKeyboardKey_V = 86, //!< Letter V key.
    HCKeyboardKey_W = 87, //!< Letter W key.
    HCKeyboardKey_X = 88, //!< Letter X key.
    HCKeyboardKey_Y = 89, //!< Letter Y key.
    HCKeyboardKey_Z = 90, //!< Letter Z key.
    HCKeyboardKey_LeftBracket = 91, //!< Left bracket key. ([)
    HCKeyboardKey_Backslash = 92, //!< Backslash key. (\)
    HCKeyboardKey_RightBracket = 93, //!< Right bracket key. (])
    HCKeyboardKey_GraveAccent = 96, //!< Grave accent key. (`)
    HCKeyboardKey_World1 = 161, //!< World key 1. (not present in US keyboard layouts)
    HCKeyboardKey_World2 = 162, //!< World key 2. (not present in US keyboard layouts)
    HCKeyboardKey_Escape = 256, //!< Escape key.
    HCKeyboardKey_Enter = 257, //!< Enter key.
    HCKeyboardKey_Tab = 258, //!< Tab key.
    HCKeyboardKey_Backspace = 259, //!< Backspace key.
    HCKeyboardKey_Insert = 260, //!< Insert key.
    HCKeyboardKey_Delete = 261, //!< Delete key.
    HCKeyboardKey_Right = 262, //!< Right arrow key.
    HCKeyboardKey_Left = 263, //!< Left arrow key.
    HCKeyboardKey_Down = 264, //!< Down arrow key.
    HCKeyboardKey_Up = 265, //!< Up arrow key.
    HCKeyboardKey_PageUp = 266, //!< Page up key.
    HCKeyboardKey_PageDown = 267, //!< Page down key.
    HCKeyboardKey_Home = 268, //!< Home key.
    HCKeyboardKey_End = 269, //!< End key.
    HCKeyboardKey_CapsLock = 280, //!< Caps lock key.
    HCKeyboardKey_ScrollLock = 281, //!< Scroll lock key.
    HCKeyboardKey_NumLock = 282, //!< Num lock key.
    HCKeyboardKey_PrintScreen = 283, //!< Print screen key.
    HCKeyboardKey_Pause = 284, //!< Pause key.
    HCKeyboardKey_F1 = 290, //!< Function 1 key. (F1)
    HCKeyboardKey_F2 = 291, //!< Function 2 key. (F2)
    HCKeyboardKey_F3 = 292, //!< Function 3 key. (F3)
    HCKeyboardKey_F4 = 293, //!< Function 4 key. (F4)
    HCKeyboardKey_F5 = 294, //!< Function 5 key. (F5)
    HCKeyboardKey_F6 = 295, //!< Function 6 key. (F6)
    HCKeyboardKey_F7 = 296, //!< Function 7 key. (F7)
    HCKeyboardKey_F8 = 297, //!< Function 8 key. (F8)
    HCKeyboardKey_F9 = 298, //!< Function 9 key. (F9)
    HCKeyboardKey_F10 = 299, //!< Function 10 key. (F10)
    HCKeyboardKey_F11 = 300, //!< Function 11 key. (F11)
    HCKeyboardKey_F12 = 301, //!< Function 12 key. (F12)
    HCKeyboardKey_F13 = 302, //!< Function 13 key. (F13)
    HCKeyboardKey_F14 = 303, //!< Function 14 key. (F14)
    HCKeyboardKey_F15 = 304, //!< Function 15 key. (F15)
    HCKeyboardKey_F16 = 305, //!< Function 16 key. (F16)
    HCKeyboardKey_F17 = 306, //!< Function 17 key. (F17)
    HCKeyboardKey_F18 = 307, //!< Function 18 key. (F18)
    HCKeyboardKey_F19 = 308, //!< Function 19 key. (F19)
    HCKeyboardKey_F20 = 309, //!< Function 20 key. (F20)
    HCKeyboardKey_F21 = 310, //!< Function 21 key. (F21)
    HCKeyboardKey_F22 = 311, //!< Function 22 key. (F22)
    HCKeyboardKey_F23 = 312, //!< Function 23 key. (F23)
    HCKeyboardKey_F24 = 313, //!< Function 24 key. (F24)
    HCKeyboardKey_F25 = 314, //!< Function 25 key. (F25)
    HCKeyboardKey_Numpad0 = 320, //!< Numpad number 0 key.
    HCKeyboardKey_Numpad1 = 321, //!< Numpad number 1 key.
    HCKeyboardKey_Numpad2 = 322, //!< Numpad number 2 key.
    HCKeyboardKey_Numpad3 = 323, //!< Numpad number 3 key.
    HCKeyboardKey_Numpad4 = 324, //!< Numpad number 4 key.
    HCKeyboardKey_Numpad5 = 325, //!< Numpad number 5 key.
    HCKeyboardKey_Numpad6 = 326, //!< Numpad number 6 key.
    HCKeyboardKey_Numpad7 = 327, //!< Numpad number 7 key.
    HCKeyboardKey_Numpad8 = 328, //!< Numpad number 8 key.
    HCKeyboardKey_Numpad9 = 329, //!< Numpad number 9 key.
    HCKeyboardKey_NumpadDecimal = 330, //!< Numpad decimal key.
    HCKeyboardKey_NumpadDivide = 331, //!< Numpad divide key.
    HCKeyboardKey_NumpadMultiply = 332, //!< Numpad multiply key.
    HCKeyboardKey_NumpadSubtract = 333, //!< Numpad subtract key.
    HCKeyboardKey_NumpadAdd = 334, //!< Numpad add key.
    HCKeyboardKey_NumpadEnter = 335, //!< Numpad enter key.
    HCKeyboardKey_NumpadEqual = 336, //!< Numpad equal key.
    HCKeyboardKey_LeftShift = 340, //!< Left shift key.
    HCKeyboardKey_LeftControl = 341, //!< Left control key.
    HCKeyboardKey_LeftAlt = 342, //!< Left alt key.
    HCKeyboardKey_LeftSuper = 343, //!< Left super key.
    HCKeyboardKey_RightShift = 344, //!< Right shift key.
    HCKeyboardKey_RightControl = 345, //!< Right control key.
    HCKeyboardKey_RightAlt = 346, //!< Right alt key.
    HCKeyboardKey_RightSuper = 347, //!< Right super key.
    HCKeyboardKey_Menu = 348, //!< Menu key.
};

/**
 * Events related to devices external to the system.
 */
enum HCDeviceEvent {
    HCDeviceEvent_Connected, //!< A device as been connected to the system.
    HCDeviceEvent_Disconnected, //!< A device has been disconnected from the system.
};

// Callback definitions

/**
 * @brief The type/signature of window position callback functions.
 *
 * This kind of function is typically called when a window is moved.
 *
 * @param window The window identifier.
 * @param x The new x-coordinate, in screen coordinates, of the upper-left corner of the content area of the window.
 * @param y The new y-coordinate, in screen coordinates, of the upper-left corner of the content area of the window.
 */
typedef void (*HCWindowPositionCallback)(size_t window, int x, int y);

/**
 * @brief The type/signature of window size callback functions.
 *
 * This kind of function is typically called when a window is resized.
 *
 * @param window The window identifier.
 * @param width The new width, in screen coordinates, of the window.
 * @param height The new height, in screen coordinates, of the window.
 */
typedef void (*HCWindowSizeCallback)(size_t window, int width, int height);

/**
 * @brief The type/signature of window close callback functions.
 *
 * This kind of function is typically called, for example, when a user clicks the window's close button.
 *
 * @param window The window identifier.
 */
typedef void (*HCWindowCloseCallback)(size_t window);

/**
 * @brief The type/signature of window refresh callback functions.
 *
 * This kind of function is typically called when the contents of the window need to be refreshed after getting damaged.
 *
 * @param window The window identifier.
 */
typedef void (*HCWindowRefreshCallback)(size_t window);

/**
 * @brief The type/signature of window focus callback functions.
 *
 * This kind of function is typically called when the window comes into or out of focus.
 *
 * @param window The window identifier.
 * @param focused `true` if the window was given input focus, or `false` if it lost it.
 */
typedef void (*HCWindowFocusCallback)(size_t window, bool focused);

/**
 * @brief The type/signature of window minimize/iconify callback functions.
 *
 * This kind of function is typically called when the window is minimized/iconified or restored.
 *
 * @param window The window identifier.
 * @param minimized  `true` if the window was minimized, or `false` if it was restored.
 */
typedef void (*HCWindowMinimizeCallback)(size_t window, bool minimized);

/**
 * @brief The type/signature of window maximize callback functions.
 *
 * This kind of function is typically called when the window is maximized or restored.
 *
 * @param window The window identifier.
 * @param maximized  `true` if the window was maximized, or `false` if it was restored.
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
 * @param window The window identifier.
 * @param width The new width, in pixels, of the framebuffer.
 * @param height The new height, in pixels, of the framebuffer.
 */
typedef void (*HCWindowFramebufferCallback)(size_t window, int width, int height);

/**
 * @brief The type/signature of window scale callback functions.
 *
 * This kind of function is typically called when a window is rescaled.
 *
 * @param window The window identifier.
 * @param x_scale The new x-axis content scale of the window.
 * @param y_scale The new y-axis content scale of the window.
 */
typedef void (*HCWindowScaleCallback)(size_t window, float x_scale, float y_scale);

/**
 * @brief The type/signature of mouse button callback functions.
 *
 * This kind of function is typically called when a mouse button is pressed or released.
 *
 * @param window The window identifier.
 * @param button The mouse button that was pressed or released.
 * @param action Either `HCButtonAction::Press` or `HCButtonAction::Release`. Future releases may add more actions.
 * @param mods Bit field describing which modifiers are active.
 */
typedef void (*HCWindowMouseButtonCallback)(
    size_t window,
    enum HCMouseButton button,
    enum HCButtonAction action,
    int mods
);

/**
 * @brief The type/signature of cursor position callback functions.
 *
 * This kind of function is typically called when the mouse cursor is moved.
 *
 * @param window The window identifier.
 * @param x The new cursor x-coordinate, relative to the left edge of the content area.
 * @param y The new cursor y-coordinate, relative to the top edge of the content area.
 */
typedef void (*HCWindowCursorPositionCallback)(size_t window, double x, double y);

/**
 * @brief The type/signature of cursor enter/leave callback functions.
 *
 * This kind of function is typically called when the mouse cursor enters or leaves the content area.
 *
 * @param window The window identifier.
 * @param entered  `true` if the cursor entered the window's content area, or `false` if it left it.
 */
typedef void (*HCWindowCursorEnterCallback)(size_t window, bool entered);

/**
 * @brief The type/signature of scroll callback functions.
 *
 * This kind of function is typically called when a user, for example, uses the scroll wheel.
 *
 * @param window The window identifier.
 * @param x_offset The scroll offset along the x-axis.
 * @param y_offset The scroll offset along the y-axis.
 */
typedef void (*HCWindowScrollCallback)(size_t window, double x_offset, double y_offset);

/**
 * @brief The type/signature of keyboard key callback functions.
 *
 * This kind of function is typically called when a keyboard key is pressed, released or held (repeated).
 *
 * @param window The window identifier.
 * @param key The keyboard key that was pressed or released.
 * @param scan_code The platform-specific scancode of the key.
 * @param action `HCButtonAction::Press`, `HCButtonAction::Release` or `HCButtonAction::Repeat`. Future releases may
 * add more actions.
 * @param mods Bit field describing which modifiers are active.
 */
typedef void (*HCWindowKeyCallback)(
    size_t window,
    enum HCKeyboardKey key,
    int scan_code,
    enum HCButtonAction action,
    int mods
);

/**
 * @brief The type/signature of Unicode character callback functions.
 *
 * This kind of function is typically called when a character key is pressed, released or held (repeated), essentially,
 * when something is typed.
 *
 * @param window The window identifier.
 * @param code_point The Unicode code point of the character.
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
 * @param window The window identifier.
 * @param code_point The Unicode code point of the character.
 * @param mods Bit field describing which modifiers are active.
 */
typedef void (*HCWindowCharModsCallback)(size_t window, unsigned int code_point, int mods);

/**
 * @brief The type/signature of path drop callback functions.
 *
 * This kind of function is typically called when a selection is dropped inside the content area.
 *
 * @param window The window identifier.
 * @param path_count The number of dropped paths.
 * @param paths The UTF-8 encoded file and/or directory path names.
 */
typedef void (*HCWindowDropCallback)(size_t window, int path_count, const char* paths[]);

/**
 * @brief The type/signature of monitor configuration callback functions.
 *
 * This kind of function is typically called when a monitor is connected or disconnected
 *
 * @param window The window identifier.
 * @param event One of `HCDeviceEvent::Connected` or `HCDeviceEvent::Disconnected`. Future releases may add more events.
 */
typedef void (*HCWindowMonitorCallback)(size_t window, enum HCDeviceEvent event);

/**
 * @brief The type/signature of joystick configuration callback functions.
 *
 * This kind of function is typically called when a joystick is connected or disconnected
 *
 * @param window The window identifier.
 * @param event One of `HCDeviceEvent::Connected` or `HCDeviceEvent::Disconnected`. Future releases may add more events.
 */
typedef void (*HCWindowJoystickCallback)(size_t window, enum HCDeviceEvent event);

// Window type and function declarations

/**
 * @brief A high-level abstraction of a graphical window.
 *
 * This struct should never be created directly. Instead, `hc_new_window` should be used to create a new instance, and
 * `hc_destroy_window` used to destroy the instance.
 */
struct HCWindow {
    void* handle; //!< The internal handle of this window.
    size_t id; //!< The global id of this window.
};

/**
 * @brief Initialisation parameters for a new window.
 */
struct HCWindowParams {
    uint32_t device; //!< The ID of the device in which this window's buffer will be allocated in.
    uint32_t width; //!< The width of the window.
    uint32_t height; //!< The height of the window.
    int pos_x; //!< A hint for the initial position of the window, in the x-axis.
    int pos_y; //!< A hint for the initial position of the window, in the y-axis.
    const char* name; //!< The name of the window, UTF-8 encoded.
};

/**
 * @brief Processes all pending window events.
 *
 * This function is meant to be called in a loop, once per frame.
 * It MUST be called in non-headless configurations, and it MUST be from the main thread.
 */
void hc_poll_events();

/**
 * @brief Constructs a new `HCWindow`.
 *
 * @param params The window initialisation parameters.
 *
 * @return The newly created `HCWindow`. The returned object is invalid if some error has occurred.
 */
struct HCWindow hc_new_window(struct HCWindowParams params);

/**
 * @brief Destroys a `HCWindow`.
 *
 * The window will become unusable immediately, but will only truly be destroyed once its resources are no longer
 * being used, which should be after a few frames, depending on the number of frames in flight.
 *
 * @param window A pointer to the `HCWindow` to be destroyed.
 */
void hc_destroy_window(struct HCWindow* window);


/**
 * @brief Set the cursor mode of a window.
 *
 * @param window A pointer to the `HCWindow` for which the cursor mode is set.
 * @param cursor_mode The cursor mode to change into.
 */
void hc_set_window_cursor_mode(struct HCWindow* window, enum HCCursorMode cursor_mode);

/**
 * @brief Sets the window position callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window A pointer to the `HCWindow` for which the callback is set.
 * @param callback A pointer to the new callback function.
 */
void hc_set_window_position_callback(struct HCWindow* window, HCWindowPositionCallback callback);

/**
 * @brief Sets the window size callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_size_callback(struct HCWindow* window, HCWindowSizeCallback callback);

/**
 * @brief Sets the window close callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_close_callback(struct HCWindow* window, HCWindowCloseCallback callback);

/**
 * @brief Sets the window refresh callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_refresh_callback(struct HCWindow* window, HCWindowRefreshCallback callback);

/**
 * @brief Sets the window focus callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_focus_callback(struct HCWindow* window, HCWindowFocusCallback callback);

/**
 * @brief Sets the window minimize callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_minimize_callback(struct HCWindow* window, HCWindowMinimizeCallback callback);

/**
 * @brief Sets the window maximize callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_maximize_callback(struct HCWindow* window, HCWindowMaximizeCallback callback);

/**
 * @brief Sets the window framebuffer callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_framebuffer_callback(struct HCWindow* window, HCWindowFramebufferCallback callback);

/**
 * @brief Sets the window scale callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_scale_callback(struct HCWindow* window, HCWindowScaleCallback callback);

/**
 * @brief Sets the mouse button callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_mouse_button_callback(struct HCWindow* window, HCWindowMouseButtonCallback callback);

/**
 * @brief Sets the cursor position callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_cursor_position_callback(struct HCWindow* window, HCWindowCursorPositionCallback callback);

/**
 * @brief Sets the cursor enter callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_cursor_enter_callback(struct HCWindow* window, HCWindowCursorEnterCallback callback);

/**
 * @brief Sets the scroll callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_scroll_callback(struct HCWindow* window, HCWindowScrollCallback callback);

/**
 * @brief Sets the key callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_key_callback(struct HCWindow* window, HCWindowKeyCallback callback);

/**
 * @brief Sets the character callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_char_callback(struct HCWindow* window, HCWindowCharCallback callback);

/**
 * @brief Sets the character with modifiers callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_char_mods_callback(struct HCWindow* window, HCWindowCharModsCallback callback);

/**
 * @brief Sets the window drop callback.
 *
 * If the pointer to the new callback is null, unsets the callback and none is used.
 *
 * @param window a pointer to the `HCWindow` for which the callback is set.
 * @param callback a pointer to the new callback function.
 */
void hc_set_window_drop_callback(struct HCWindow* window, HCWindowDropCallback callback);

//void hc_set_window_monitor_callback(struct HCWindow *window, HCWindowMonitorCallback callback);
//
//void hc_set_window_joystick_callback(struct HCWindow *window, HCWindowJoystickCallback callback);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HC_HEADLESS
