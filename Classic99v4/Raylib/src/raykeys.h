// just the key defines from raylib

#ifndef RAYKEYS_H
#define RAYKEYS_H

#if defined(__cplusplus)
extern "C" {
#endif

// Keyboard keys (US keyboard layout)
// NOTE: Use GetKeyPressed() to allow redefining
// required keys for alternative layouts
// (mb) all KEY_xxx enums renamed to RL_KEY_xxx, to avoid conflicting with curses keys
typedef enum {
    RL_KEY_NULL            = 0,        // Key: NULL, used for no key pressed
    // Alphanumeric keys
    RL_KEY_APOSTROPHE      = 39,       // Key: '
    RL_KEY_COMMA           = 44,       // Key: ,
    RL_KEY_MINUS           = 45,       // Key: -
    RL_KEY_PERIOD          = 46,       // Key: .
    RL_KEY_SLASH           = 47,       // Key: /
    RL_KEY_ZERO            = 48,       // Key: 0
    RL_KEY_ONE             = 49,       // Key: 1
    RL_KEY_TWO             = 50,       // Key: 2
    RL_KEY_THREE           = 51,       // Key: 3
    RL_KEY_FOUR            = 52,       // Key: 4
    RL_KEY_FIVE            = 53,       // Key: 5
    RL_KEY_SIX             = 54,       // Key: 6
    RL_KEY_SEVEN           = 55,       // Key: 7
    RL_KEY_EIGHT           = 56,       // Key: 8
    RL_KEY_NINE            = 57,       // Key: 9
    RL_KEY_SEMICOLON       = 59,       // Key: ;
    RL_KEY_EQUAL           = 61,       // Key: =
    RL_KEY_A               = 65,       // Key: A | a
    RL_KEY_B               = 66,       // Key: B | b
    RL_KEY_C               = 67,       // Key: C | c
    RL_KEY_D               = 68,       // Key: D | d
    RL_KEY_E               = 69,       // Key: E | e
    RL_KEY_F               = 70,       // Key: F | f
    RL_KEY_G               = 71,       // Key: G | g
    RL_KEY_H               = 72,       // Key: H | h
    RL_KEY_I               = 73,       // Key: I | i
    RL_KEY_J               = 74,       // Key: J | j
    RL_KEY_K               = 75,       // Key: K | k
    RL_KEY_L               = 76,       // Key: L | l
    RL_KEY_M               = 77,       // Key: M | m
    RL_KEY_N               = 78,       // Key: N | n
    RL_KEY_O               = 79,       // Key: O | o
    RL_KEY_P               = 80,       // Key: P | p
    RL_KEY_Q               = 81,       // Key: Q | q
    RL_KEY_R               = 82,       // Key: R | r
    RL_KEY_S               = 83,       // Key: S | s
    RL_KEY_T               = 84,       // Key: T | t
    RL_KEY_U               = 85,       // Key: U | u
    RL_KEY_V               = 86,       // Key: V | v
    RL_KEY_W               = 87,       // Key: W | w
    RL_KEY_X               = 88,       // Key: X | x
    RL_KEY_Y               = 89,       // Key: Y | y
    RL_KEY_Z               = 90,       // Key: Z | z
    RL_KEY_LEFT_BRACKET    = 91,       // Key: [
    RL_KEY_BACKSLASH       = 92,       // Key: '\'
    RL_KEY_RIGHT_BRACKET   = 93,       // Key: ]
    RL_KEY_GRAVE           = 96,       // Key: `
    // Function keys
    RL_KEY_SPACE           = 32,       // Key: Space
    RL_KEY_ESCAPE          = 256,      // Key: Esc
    RL_KEY_ENTER           = 257,      // Key: Enter
    RL_KEY_TAB             = 258,      // Key: Tab
    RL_KEY_BACKSPACE       = 259,      // Key: Backspace
    RL_KEY_INSERT          = 260,      // Key: Ins
    RL_KEY_DELETE          = 261,      // Key: Del
    RL_KEY_RIGHT           = 262,      // Key: Cursor right
    RL_KEY_LEFT            = 263,      // Key: Cursor left
    RL_KEY_DOWN            = 264,      // Key: Cursor down
    RL_KEY_UP              = 265,      // Key: Cursor up
    RL_KEY_PAGE_UP         = 266,      // Key: Page up
    RL_KEY_PAGE_DOWN       = 267,      // Key: Page down
    RL_KEY_HOME            = 268,      // Key: Home
    RL_KEY_END             = 269,      // Key: End
    RL_KEY_CAPS_LOCK       = 280,      // Key: Caps lock
    RL_KEY_SCROLL_LOCK     = 281,      // Key: Scroll down
    RL_KEY_NUM_LOCK        = 282,      // Key: Num lock
    RL_KEY_PRINT_SCREEN    = 283,      // Key: Print screen
    RL_KEY_PAUSE           = 284,      // Key: Pause
    RL_KEY_F1              = 290,      // Key: F1
    RL_KEY_F2              = 291,      // Key: F2
    RL_KEY_F3              = 292,      // Key: F3
    RL_KEY_F4              = 293,      // Key: F4
    RL_KEY_F5              = 294,      // Key: F5
    RL_KEY_F6              = 295,      // Key: F6
    RL_KEY_F7              = 296,      // Key: F7
    RL_KEY_F8              = 297,      // Key: F8
    RL_KEY_F9              = 298,      // Key: F9
    RL_KEY_F10             = 299,      // Key: F10
    RL_KEY_F11             = 300,      // Key: F11
    RL_KEY_F12             = 301,      // Key: F12
    RL_KEY_LEFT_SHIFT      = 340,      // Key: Shift left
    RL_KEY_LEFT_CONTROL    = 341,      // Key: Control left
    RL_KEY_LEFT_ALT        = 342,      // Key: Alt left
    RL_KEY_LEFT_SUPER      = 343,      // Key: Super left
    RL_KEY_RIGHT_SHIFT     = 344,      // Key: Shift right
    RL_KEY_RIGHT_CONTROL   = 345,      // Key: Control right
    RL_KEY_RIGHT_ALT       = 346,      // Key: Alt right
    RL_KEY_RIGHT_SUPER     = 347,      // Key: Super right
    RL_KEY_KB_MENU         = 348,      // Key: KB menu
    // Keypad keys
    RL_KEY_KP_0            = 320,      // Key: Keypad 0
    RL_KEY_KP_1            = 321,      // Key: Keypad 1
    RL_KEY_KP_2            = 322,      // Key: Keypad 2
    RL_KEY_KP_3            = 323,      // Key: Keypad 3
    RL_KEY_KP_4            = 324,      // Key: Keypad 4
    RL_KEY_KP_5            = 325,      // Key: Keypad 5
    RL_KEY_KP_6            = 326,      // Key: Keypad 6
    RL_KEY_KP_7            = 327,      // Key: Keypad 7
    RL_KEY_KP_8            = 328,      // Key: Keypad 8
    RL_KEY_KP_9            = 329,      // Key: Keypad 9
    RL_KEY_KP_DECIMAL      = 330,      // Key: Keypad .
    RL_KEY_KP_DIVIDE       = 331,      // Key: Keypad /
    RL_KEY_KP_MULTIPLY     = 332,      // Key: Keypad *
    RL_KEY_KP_SUBTRACT     = 333,      // Key: Keypad -
    RL_KEY_KP_ADD          = 334,      // Key: Keypad +
    RL_KEY_KP_ENTER        = 335,      // Key: Keypad Enter
    RL_KEY_KP_EQUAL        = 336,      // Key: Keypad =
    // Android key buttons
    RL_KEY_BACK            = 4,        // Key: Android back button
    RL_KEY_MENU            = 5,        // Key: Android menu button
    RL_KEY_VOLUME_UP       = 24,       // Key: Android volume up button
    RL_KEY_VOLUME_DOWN     = 25        // Key: Android volume down button
} KeyboardKey;

#if defined(__cplusplus)
}
#endif

#endif // RAYKEYS_H
