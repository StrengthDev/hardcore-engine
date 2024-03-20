use std::fmt::{Debug, Formatter};

/// A button input action.
#[derive(Debug, Eq, PartialEq)]
pub enum ButtonAction {
    /// Stop pressing a button.
    Release,
    /// Begin pressing of a button.
    Press,
    /// The repeated input which happens when a button is being held down.
    Repeat,
}

impl ButtonAction {
    pub(crate) fn from(action: hardcore_sys::ButtonAction) -> Self {
        match action {
            hardcore_sys::ButtonAction::Release => ButtonAction::Release,
            hardcore_sys::ButtonAction::Press => ButtonAction::Press,
            hardcore_sys::ButtonAction::Repeat => ButtonAction::Repeat,
            _ => {
                unimplemented!()
            }
        }
    }
}

/// An identifier for a mouse button.
#[derive(Debug, Eq, PartialEq)]
pub enum MouseButton {
    /// Mouse button 0, also known as the left mouse button.
    Button0,

    /// Mouse button 1, also known as the right mouse button.
    Button1,

    /// Mouse button 2, also known as the middle mouse button.
    Button2,

    /// Mouse button 3.
    Button3,

    /// Mouse button 4.
    Button4,

    /// Mouse button 5.
    Button5,

    /// Mouse button 6.
    Button6,

    /// Mouse button 7.
    Button7,
}

impl MouseButton {
    /// The left mouse button, an alias for mouse button 0.
    #[allow(non_upper_case_globals)]
    pub const Left: MouseButton = MouseButton::Button0;

    /// The right mouse button, an alias for mouse button 1.
    #[allow(non_upper_case_globals)]
    pub const Right: MouseButton = MouseButton::Button1;

    /// The middle mouse button, an alias for mouse button 2.
    #[allow(non_upper_case_globals)]
    pub const Middle: MouseButton = MouseButton::Button2;

    pub(crate) fn from(button: hardcore_sys::MouseButton) -> Self {
        match button {
            hardcore_sys::MouseButton::Button0 => MouseButton::Button0,
            hardcore_sys::MouseButton::Button1 => MouseButton::Button1,
            hardcore_sys::MouseButton::Button2 => MouseButton::Button2,
            hardcore_sys::MouseButton::Button3 => MouseButton::Button3,
            hardcore_sys::MouseButton::Button4 => MouseButton::Button4,
            hardcore_sys::MouseButton::Button5 => MouseButton::Button5,
            hardcore_sys::MouseButton::Button6 => MouseButton::Button6,
            hardcore_sys::MouseButton::Button7 => MouseButton::Button7,
            _ => {
                unimplemented!()
            }
        }
    }
}

#[derive(Eq, PartialEq)]
pub struct Modifiers {
    flags: i32,
}

impl Modifiers {
    pub(crate) fn from(flags: i32) -> Self {
        Self { flags }
    }

    fn check_flag(&self, flag: i32) -> bool {
        (self.flags & flag).is_positive()
    }

    /// Return true if one or more Shift keys were held down.
    pub fn shift(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_SHIFT)
    }

    /// Return true if one or more Control keys were held down.
    pub fn ctrl(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_CONTROL)
    }

    /// Return true if one or more Alt keys were held down.
    pub fn alt(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_ALT)
    }

    /// Return true if one or more Super keys were held down.
    pub fn command(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_SUPER)
    }

    /// Return true if he Caps Lock key is enabled.
    pub fn caps_lock(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_CAPS_LOCK)
    }

    /// Return true if the Num Lock key is enabled.
    pub fn num_lock(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_NUM_LOCK)
    }
}

impl Debug for Modifiers {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let mut flags = Vec::with_capacity(6);

        if self.shift() {
            flags.push("Shift")
        }

        if self.ctrl() {
            flags.push("Ctrl")
        }

        if self.alt() {
            flags.push("Alt")
        }

        if self.command() {
            flags.push("Super")
        }

        if self.caps_lock() {
            flags.push("Caps lock")
        }

        if self.num_lock() {
            flags.push("Num lock")
        }

        write!(f, "({})", flags.join("|"))
    }
}

/// Keyboard key identifiers.
///
/// The US keyboard layout is used.
#[derive(Debug, Eq, PartialEq)]
pub enum KeyboardKey {
    /// Space key.
    Space,

    /// Apostrophe key. (')
    Apostrophe,

    /// Comma key. (,)
    Comma,

    /// Minus key. (-)
    Minus,

    /// Period key. (.)
    Period,

    /// Slash key. (/)
    Slash,

    /// Number 0 key.
    Num0,

    /// Number 1 key.
    Num1,

    /// Number 2 key.
    Num2,

    /// Number 3 key.
    Num3,

    /// Number 4 key.
    Num4,

    /// Number 5 key.
    Num5,

    /// Number 6 key.
    Num6,

    /// Number 7 key.
    Num7,

    /// Number 8 key.
    Num8,

    /// Number 9 key.
    Num9,

    /// Semicolon key. (;)
    Semicolon,

    /// Equal key. (=)
    Equal,

    /// Letter A key.
    A,

    /// Letter B key.
    B,

    /// Letter C key.
    C,

    /// Letter D key.
    D,

    /// Letter E key.
    E,

    /// Letter F key.
    F,

    /// Letter G key.
    G,

    /// Letter H key.
    H,

    /// Letter I key.
    I,

    /// Letter J key.
    J,

    /// Letter K key.
    K,

    /// Letter L key.
    L,

    /// Letter M key.
    M,

    /// Letter N key.
    N,

    /// Letter O key.
    O,

    /// Letter P key.
    P,

    /// Letter Q key.
    Q,

    /// Letter R key.
    R,

    /// Letter S key.
    S,

    /// Letter T key.
    T,

    /// Letter U key.
    U,

    /// Letter V key.
    V,

    /// Letter W key.
    W,

    /// Letter X key.
    X,

    /// Letter Y key.
    Y,

    /// Letter Z key.
    Z,

    /// Left bracket key. ([)
    LeftBracket,

    /// Backslash key. (\)
    Backslash,

    /// Right bracket key. (])
    RightBracket,

    /// Grave accent key. (`)
    GraveAccent,

    /// World key 1. (not present in US keyboard layouts)
    World1,

    /// World key 2. (not present in US keyboard layouts)
    World2,

    /// Escape key.
    Escape,

    /// Enter key.
    Enter,

    /// Tab key.
    Tab,

    /// Backspace key.
    Backspace,

    /// Insert key.
    Insert,

    /// Delete key.
    Delete,

    /// Right arrow key.
    Right,

    /// Left arrow key.
    Left,

    /// Down arrow key.
    Down,

    /// Up arrow key.
    Up,

    /// Page up key.
    PageUp,

    /// Page down key.
    PageDown,

    /// Home key.
    Home,

    /// End key.
    End,

    /// Caps lock key.
    CapsLock,

    /// Scroll lock key.
    ScrollLock,

    /// Num lock key.
    NumLock,

    /// Print screen key.
    PrintScreen,

    /// Pause key.
    Pause,

    /// Function 1 key. (F1)
    F1,

    /// Function 2 key. (F2)
    F2,

    /// Function 3 key. (F3)
    F3,

    /// Function 4 key. (F4)
    F4,

    /// Function 5 key. (F5)
    F5,

    /// Function 6 key. (F6)
    F6,

    /// Function 7 key. (F7)
    F7,

    /// Function 8 key. (F8)
    F8,

    /// Function 9 key. (F9)
    F9,

    /// Function 10 key. (F10)
    F10,

    /// Function 11 key. (F11)
    F11,

    /// Function 12 key. (F12)
    F12,

    /// Function 13 key. (F13)
    F13,

    /// Function 14 key. (F14)
    F14,

    /// Function 15 key. (F15)
    F15,

    /// Function 16 key. (F16)
    F16,

    /// Function 17 key. (F17)
    F17,

    /// Function 18 key. (F18)
    F18,

    /// Function 19 key. (F19)
    F19,

    /// Function 20 key. (F20)
    F20,

    /// Function 21 key. (F21)
    F21,

    /// Function 22 key. (F22)
    F22,

    /// Function 23 key. (F23)
    F23,

    /// Function 24 key. (F24)
    F24,

    /// Function 25 key. (F25)
    F25,

    /// Numpad number 0 key.
    Numpad0,

    /// Numpad number 1 key.
    Numpad1,

    /// Numpad number 2 key.
    Numpad2,

    /// Numpad number 3 key.
    Numpad3,

    /// Numpad number 4 key.
    Numpad4,

    /// Numpad number 5 key.
    Numpad5,

    /// Numpad number 6 key.
    Numpad6,

    /// Numpad number 7 key.
    Numpad7,

    /// Numpad number 8 key.
    Numpad8,

    /// Numpad number 9 key.
    Numpad9,

    /// Numpad decimal key.
    NumpadDecimal,

    /// Numpad divide key.
    NumpadDivide,

    /// Numpad multiply key.
    NumpadMultiply,

    /// Numpad subtract key.
    NumpadSubtract,

    /// Numpad add key.
    NumpadAdd,

    /// Numpad enter key.
    NumpadEnter,

    /// Numpad equal key.
    NumpadEqual,

    /// Left shift key.
    LeftShift,

    /// Left control key.
    LeftControl,

    /// Left alt key.
    LeftAlt,

    /// Left super key.
    LeftSuper,

    /// Right shift key.
    RightShift,

    /// Right control key.
    RightControl,

    /// Right alt key.
    RightAlt,

    /// Right super key.
    RightSuper,

    /// Menu key.
    Menu,
}

impl KeyboardKey {
    pub(crate) fn from(key: hardcore_sys::KeyboardKey) -> Self {
        match key {
            hardcore_sys::KeyboardKey::Space => KeyboardKey::Space,
            hardcore_sys::KeyboardKey::Apostrophe => KeyboardKey::Apostrophe,
            hardcore_sys::KeyboardKey::Comma => KeyboardKey::Comma,
            hardcore_sys::KeyboardKey::Minus => KeyboardKey::Minus,
            hardcore_sys::KeyboardKey::Period => KeyboardKey::Period,
            hardcore_sys::KeyboardKey::Slash => KeyboardKey::Slash,
            hardcore_sys::KeyboardKey::Num0 => KeyboardKey::Num0,
            hardcore_sys::KeyboardKey::Num1 => KeyboardKey::Num1,
            hardcore_sys::KeyboardKey::Num2 => KeyboardKey::Num2,
            hardcore_sys::KeyboardKey::Num3 => KeyboardKey::Num3,
            hardcore_sys::KeyboardKey::Num4 => KeyboardKey::Num4,
            hardcore_sys::KeyboardKey::Num5 => KeyboardKey::Num5,
            hardcore_sys::KeyboardKey::Num6 => KeyboardKey::Num6,
            hardcore_sys::KeyboardKey::Num7 => KeyboardKey::Num7,
            hardcore_sys::KeyboardKey::Num8 => KeyboardKey::Num8,
            hardcore_sys::KeyboardKey::Num9 => KeyboardKey::Num9,
            hardcore_sys::KeyboardKey::Semicolon => KeyboardKey::Semicolon,
            hardcore_sys::KeyboardKey::Equal => KeyboardKey::Equal,
            hardcore_sys::KeyboardKey::A => KeyboardKey::A,
            hardcore_sys::KeyboardKey::B => KeyboardKey::B,
            hardcore_sys::KeyboardKey::C => KeyboardKey::C,
            hardcore_sys::KeyboardKey::D => KeyboardKey::D,
            hardcore_sys::KeyboardKey::E => KeyboardKey::E,
            hardcore_sys::KeyboardKey::F => KeyboardKey::F,
            hardcore_sys::KeyboardKey::G => KeyboardKey::G,
            hardcore_sys::KeyboardKey::H => KeyboardKey::H,
            hardcore_sys::KeyboardKey::I => KeyboardKey::I,
            hardcore_sys::KeyboardKey::J => KeyboardKey::J,
            hardcore_sys::KeyboardKey::K => KeyboardKey::K,
            hardcore_sys::KeyboardKey::L => KeyboardKey::L,
            hardcore_sys::KeyboardKey::M => KeyboardKey::M,
            hardcore_sys::KeyboardKey::N => KeyboardKey::N,
            hardcore_sys::KeyboardKey::O => KeyboardKey::O,
            hardcore_sys::KeyboardKey::P => KeyboardKey::P,
            hardcore_sys::KeyboardKey::Q => KeyboardKey::Q,
            hardcore_sys::KeyboardKey::R => KeyboardKey::R,
            hardcore_sys::KeyboardKey::S => KeyboardKey::S,
            hardcore_sys::KeyboardKey::T => KeyboardKey::T,
            hardcore_sys::KeyboardKey::U => KeyboardKey::U,
            hardcore_sys::KeyboardKey::V => KeyboardKey::V,
            hardcore_sys::KeyboardKey::W => KeyboardKey::W,
            hardcore_sys::KeyboardKey::X => KeyboardKey::X,
            hardcore_sys::KeyboardKey::Y => KeyboardKey::Y,
            hardcore_sys::KeyboardKey::Z => KeyboardKey::Z,
            hardcore_sys::KeyboardKey::LeftBracket => KeyboardKey::LeftBracket,
            hardcore_sys::KeyboardKey::Backslash => KeyboardKey::Backslash,
            hardcore_sys::KeyboardKey::RightBracket => KeyboardKey::RightBracket,
            hardcore_sys::KeyboardKey::GraveAccent => KeyboardKey::GraveAccent,
            hardcore_sys::KeyboardKey::World1 => KeyboardKey::World1,
            hardcore_sys::KeyboardKey::World2 => KeyboardKey::World2,
            hardcore_sys::KeyboardKey::Escape => KeyboardKey::Escape,
            hardcore_sys::KeyboardKey::Enter => KeyboardKey::Enter,
            hardcore_sys::KeyboardKey::Tab => KeyboardKey::Tab,
            hardcore_sys::KeyboardKey::Backspace => KeyboardKey::Backspace,
            hardcore_sys::KeyboardKey::Insert => KeyboardKey::Insert,
            hardcore_sys::KeyboardKey::Delete => KeyboardKey::Delete,
            hardcore_sys::KeyboardKey::Right => KeyboardKey::Right,
            hardcore_sys::KeyboardKey::Left => KeyboardKey::Left,
            hardcore_sys::KeyboardKey::Down => KeyboardKey::Down,
            hardcore_sys::KeyboardKey::Up => KeyboardKey::Up,
            hardcore_sys::KeyboardKey::PageUp => KeyboardKey::PageUp,
            hardcore_sys::KeyboardKey::PageDown => KeyboardKey::PageDown,
            hardcore_sys::KeyboardKey::Home => KeyboardKey::Home,
            hardcore_sys::KeyboardKey::End => KeyboardKey::End,
            hardcore_sys::KeyboardKey::CapsLock => KeyboardKey::CapsLock,
            hardcore_sys::KeyboardKey::ScrollLock => KeyboardKey::ScrollLock,
            hardcore_sys::KeyboardKey::NumLock => KeyboardKey::NumLock,
            hardcore_sys::KeyboardKey::PrintScreen => KeyboardKey::PrintScreen,
            hardcore_sys::KeyboardKey::Pause => KeyboardKey::Pause,
            hardcore_sys::KeyboardKey::F1 => KeyboardKey::F1,
            hardcore_sys::KeyboardKey::F2 => KeyboardKey::F2,
            hardcore_sys::KeyboardKey::F3 => KeyboardKey::F3,
            hardcore_sys::KeyboardKey::F4 => KeyboardKey::F4,
            hardcore_sys::KeyboardKey::F5 => KeyboardKey::F5,
            hardcore_sys::KeyboardKey::F6 => KeyboardKey::F6,
            hardcore_sys::KeyboardKey::F7 => KeyboardKey::F7,
            hardcore_sys::KeyboardKey::F8 => KeyboardKey::F8,
            hardcore_sys::KeyboardKey::F9 => KeyboardKey::F9,
            hardcore_sys::KeyboardKey::F10 => KeyboardKey::F10,
            hardcore_sys::KeyboardKey::F11 => KeyboardKey::F11,
            hardcore_sys::KeyboardKey::F12 => KeyboardKey::F12,
            hardcore_sys::KeyboardKey::F13 => KeyboardKey::F13,
            hardcore_sys::KeyboardKey::F14 => KeyboardKey::F14,
            hardcore_sys::KeyboardKey::F15 => KeyboardKey::F15,
            hardcore_sys::KeyboardKey::F16 => KeyboardKey::F16,
            hardcore_sys::KeyboardKey::F17 => KeyboardKey::F17,
            hardcore_sys::KeyboardKey::F18 => KeyboardKey::F18,
            hardcore_sys::KeyboardKey::F19 => KeyboardKey::F19,
            hardcore_sys::KeyboardKey::F20 => KeyboardKey::F20,
            hardcore_sys::KeyboardKey::F21 => KeyboardKey::F21,
            hardcore_sys::KeyboardKey::F22 => KeyboardKey::F22,
            hardcore_sys::KeyboardKey::F23 => KeyboardKey::F23,
            hardcore_sys::KeyboardKey::F24 => KeyboardKey::F24,
            hardcore_sys::KeyboardKey::F25 => KeyboardKey::F25,
            hardcore_sys::KeyboardKey::Numpad0 => KeyboardKey::Numpad0,
            hardcore_sys::KeyboardKey::Numpad1 => KeyboardKey::Numpad1,
            hardcore_sys::KeyboardKey::Numpad2 => KeyboardKey::Numpad2,
            hardcore_sys::KeyboardKey::Numpad3 => KeyboardKey::Numpad3,
            hardcore_sys::KeyboardKey::Numpad4 => KeyboardKey::Numpad4,
            hardcore_sys::KeyboardKey::Numpad5 => KeyboardKey::Numpad5,
            hardcore_sys::KeyboardKey::Numpad6 => KeyboardKey::Numpad6,
            hardcore_sys::KeyboardKey::Numpad7 => KeyboardKey::Numpad7,
            hardcore_sys::KeyboardKey::Numpad8 => KeyboardKey::Numpad8,
            hardcore_sys::KeyboardKey::Numpad9 => KeyboardKey::Numpad9,
            hardcore_sys::KeyboardKey::NumpadDecimal => KeyboardKey::NumpadDecimal,
            hardcore_sys::KeyboardKey::NumpadDivide => KeyboardKey::NumpadDivide,
            hardcore_sys::KeyboardKey::NumpadMultiply => KeyboardKey::NumpadMultiply,
            hardcore_sys::KeyboardKey::NumpadSubtract => KeyboardKey::NumpadSubtract,
            hardcore_sys::KeyboardKey::NumpadAdd => KeyboardKey::NumpadAdd,
            hardcore_sys::KeyboardKey::NumpadEnter => KeyboardKey::NumpadEnter,
            hardcore_sys::KeyboardKey::NumpadEqual => KeyboardKey::NumpadEqual,
            hardcore_sys::KeyboardKey::LeftShift => KeyboardKey::LeftShift,
            hardcore_sys::KeyboardKey::LeftControl => KeyboardKey::LeftControl,
            hardcore_sys::KeyboardKey::LeftAlt => KeyboardKey::LeftAlt,
            hardcore_sys::KeyboardKey::LeftSuper => KeyboardKey::LeftSuper,
            hardcore_sys::KeyboardKey::RightShift => KeyboardKey::RightShift,
            hardcore_sys::KeyboardKey::RightControl => KeyboardKey::RightControl,
            hardcore_sys::KeyboardKey::RightAlt => KeyboardKey::RightAlt,
            hardcore_sys::KeyboardKey::RightSuper => KeyboardKey::RightSuper,
            hardcore_sys::KeyboardKey::Menu => KeyboardKey::Menu,
            _ => {
                unimplemented!()
            }
        }
    }
}
