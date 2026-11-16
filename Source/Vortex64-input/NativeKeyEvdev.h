#pragma once
// Vortex64 1.1: the physical key of a Qt key event as the evdev code that the rest of the input code uses
// (LinuxScancodes.h turns it into the SDL scancode that is saved and shown).
//
// Linux: Qt's native scan code is the X keycode, the evdev code + 8.
// Windows: Qt's native scan code is the PC/AT set-1 scancode, with bit 0x100 for the E0 prefix of the extended keys.
// Upstream subtracted 8 on Windows too, so every key was saved and shown as the key 8 places further back
// (W -> "8", A -> "U", S -> "I", D -> "O"). For the main block of the keyboard the set-1 code IS the evdev code;
// the extended keys (arrows, Insert/Delete/Home/End/PgUp/PgDn, right Ctrl/Alt, keypad Enter and /, Windows keys)
// are translated with the table below.
#include <stdint.h>

inline int VortexNativeKeyToEvdev(uint32_t Native)
{
#ifdef _WIN32
    if ((Native & 0x100) == 0)
    {
        return (Native >= 1 && Native <= 0x58) ? (int)Native : 0;
    }
    switch (Native & 0xFF)
    {
    case 0x1C: return 96;  // keypad Enter
    case 0x1D: return 97;  // right Ctrl
    case 0x35: return 98;  // keypad /
    case 0x37: return 99;  // Print Screen
    case 0x38: return 100; // right Alt (AltGr)
    case 0x47: return 102; // Home
    case 0x48: return 103; // Up
    case 0x49: return 104; // Page Up
    case 0x4B: return 105; // Left
    case 0x4D: return 106; // Right
    case 0x4F: return 107; // End
    case 0x50: return 108; // Down
    case 0x51: return 109; // Page Down
    case 0x52: return 110; // Insert
    case 0x53: return 111; // Delete
    case 0x5B: return 125; // left Windows
    case 0x5C: return 126; // right Windows
    case 0x5D: return 127; // Menu
    default: return 0;
    }
#else
    return Native >= 8 ? (int)Native - 8 : 0;
#endif
}
