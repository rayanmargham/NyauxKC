#pragma once
#include <arch/arch.h>
#include <cppglue/glue.hpp>
#include <mem/kmem.h>
#include <stdint.h>
#include <term/term.h>
#include <utils/basic.h>


#ifdef __cplusplus
extern "C" {
#endif
enum ps2_scancode {
    PS2_SC_INVALID,
    PS2_SC_F9,
    PS2_SC_F5,
    PS2_SC_F3,
    PS2_SC_F1,
    PS2_SC_F2,
    PS2_SC_F12,
    PS2_SC_F10,
    PS2_SC_F8,
    PS2_SC_F6,
    PS2_SC_F4,

    PS2_SC_TAB,
    PS2_SC_BACKTICK,    // `
    PS2_SC_LEFT_ALT,
    PS2_SC_LEFT_SHIFT,
    PS2_SC_LEFT_CTRL,

    PS2_SC_Q,
    PS2_SC_1,
    PS2_SC_Z,
    PS2_SC_S,
    PS2_SC_A,
    PS2_SC_W,
    PS2_SC_2,
    PS2_SC_C,
    PS2_SC_X,
    PS2_SC_D,
    PS2_SC_E,
    PS2_SC_4,
    PS2_SC_3,

    PS2_SC_SPACE,
    PS2_SC_V,
    PS2_SC_F,
    PS2_SC_T,
    PS2_SC_R,
    PS2_SC_5,
    PS2_SC_N,
    PS2_SC_B,
    PS2_SC_H,
    PS2_SC_G,
    PS2_SC_Y,
    PS2_SC_6,
    PS2_SC_M,
    PS2_SC_J,
    PS2_SC_U,
    PS2_SC_7,
    PS2_SC_8,

    PS2_SC_COMMA,       // ,
    PS2_SC_K,
    PS2_SC_I,
    PS2_SC_O,
    PS2_SC_0,
    PS2_SC_9,
    PS2_SC_DOT,         // .
    PS2_SC_SLASH,       // /
    PS2_SC_L,
    PS2_SC_SEMICOLON,   // ;
    PS2_SC_P,
    PS2_SC_MINUS,       // -
    PS2_SC_APOSTROPHE,  // '
    PS2_SC_LBRACKET,    // [
    PS2_SC_EQUAL,       // =

    PS2_SC_CAPSLOCK,
    PS2_SC_RIGHT_SHIFT,
    PS2_SC_ENTER,
    PS2_SC_RBRACKET,    // ]
    PS2_SC_BACKSLASH,   // 

    PS2_SC_BACKSPACE,

    PS2_SC_KP_1,
    PS2_SC_KP_4,
    PS2_SC_KP_7,
    PS2_SC_KP_0,
    PS2_SC_KP_DOT,
    PS2_SC_KP_2,
    PS2_SC_KP_5,
    PS2_SC_KP_6,
    PS2_SC_KP_8,

    PS2_SC_ESC,
    PS2_SC_NUMLOCK,
    PS2_SC_F11,
    PS2_SC_KP_PLUS,
    PS2_SC_KP_3,
    PS2_SC_KP_MINUS,
    PS2_SC_KP_ASTERISK,
    PS2_SC_KP_9,
    PS2_SC_SCROLLLOCK,
    PS2_SC_F7,

    PS2_SC_E0_WWW_SEARCH,
    PS2_SC_E0_RIGHT_ALT,
    PS2_SC_E0_RIGHT_CTRL,
    PS2_SC_E0_PREV_TRACK,
    PS2_SC_E0_WWW_FAV,
    PS2_SC_E0_LEFT_GUI,
    PS2_SC_E0_WWW_REFRESH,
    PS2_SC_E0_VOL_DOWN,
    PS2_SC_E0_MUTE,
    PS2_SC_E0_RIGHT_GUI,
    PS2_SC_E0_WWW_STOP,
    PS2_SC_E0_CALCULATOR,
    PS2_SC_E0_APPS,
    PS2_SC_E0_WWW_FORWARD,
    PS2_SC_E0_VOL_UP,
    PS2_SC_E0_PLAY_PAUSE,
    PS2_SC_E0_POWER,
    PS2_SC_E0_WWW_BACK,
    PS2_SC_E0_WWW_HOME,
    PS2_SC_E0_STOP,
    PS2_SC_E0_SLEEP,
    PS2_SC_E0_MY_COMPUTER,
    PS2_SC_E0_EMAIL,
    PS2_SC_E0_KP_SLASH,
    PS2_SC_E0_NEXT_TRACK,
    PS2_SC_E0_MEDIA_SELECT,
    PS2_SC_E0_KP_ENTER,
    PS2_SC_E0_WAKE,
    PS2_SC_E0_END,
    PS2_SC_E0_LEFT_ARROW,
    PS2_SC_E0_HOME,
    PS2_SC_E0_INSERT,
    PS2_SC_E0_DELETE,
    PS2_SC_E0_DOWN_ARROW,
    PS2_SC_E0_RIGHT_ARROW,
    PS2_SC_E0_UP_ARROW,
    PS2_SC_E0_PAGE_DOWN,
    PS2_SC_E0_PAGE_UP,
    PS2_SC_E0_PRINT_SCREEN,

    PS2_SC_E1_PAUSE,

};
enum ps2flags {
    PRESSED,
    RELEASED,
    EXTENDEDKEYRELEASED,
    EXTENDEDKEYPRESSED,
};
typedef struct {
    char ascii;
    enum ps2_scancode keycode;
    enum ps2flags flags;
} nyauxps2kbdpacket;
static inline enum ps2_scancode ps2_scancode_from_byte(bool extended, uint8_t byte) {
    if (!extended) {
        switch (byte) {
            case 0x01: return PS2_SC_F9;
            case 0x03: return PS2_SC_F5;
            case 0x04: return PS2_SC_F3;
            case 0x05: return PS2_SC_F1;
            case 0x06: return PS2_SC_F2;
            case 0x07: return PS2_SC_F12;
            case 0x09: return PS2_SC_F10;
            case 0x0A: return PS2_SC_F8;
            case 0x0B: return PS2_SC_F6;
            case 0x0C: return PS2_SC_F4;
            case 0x0D: return PS2_SC_TAB;
            case 0x0E: return PS2_SC_BACKTICK;
            case 0x11: return PS2_SC_LEFT_ALT;
            case 0x12: return PS2_SC_LEFT_SHIFT;
            case 0x14: return PS2_SC_LEFT_CTRL;
            case 0x15: return PS2_SC_Q;
            case 0x16: return PS2_SC_1;
            case 0x1A: return PS2_SC_Z;
            case 0x1B: return PS2_SC_S;
            case 0x1C: return PS2_SC_A;
            case 0x1D: return PS2_SC_W;
            case 0x1E: return PS2_SC_2;
            case 0x21: return PS2_SC_C;
            case 0x22: return PS2_SC_X;
            case 0x23: return PS2_SC_D;
            case 0x24: return PS2_SC_E;
            case 0x25: return PS2_SC_4;
            case 0x26: return PS2_SC_3;
            case 0x29: return PS2_SC_SPACE;
            case 0x2A: return PS2_SC_V;
            case 0x2B: return PS2_SC_F;
            case 0x2C: return PS2_SC_T;
            case 0x2D: return PS2_SC_R;
            case 0x2E: return PS2_SC_5;
            case 0x31: return PS2_SC_N;
            case 0x32: return PS2_SC_B;
            case 0x33: return PS2_SC_H;
            case 0x34: return PS2_SC_G;
            case 0x35: return PS2_SC_Y;
            case 0x36: return PS2_SC_6;
            case 0x3A: return PS2_SC_M;
            case 0x3B: return PS2_SC_J;
            case 0x3C: return PS2_SC_U;
            case 0x3D: return PS2_SC_7;
            case 0x3E: return PS2_SC_8;
            case 0x41: return PS2_SC_COMMA;
            case 0x42: return PS2_SC_K;
            case 0x43: return PS2_SC_I;
            case 0x44: return PS2_SC_O;
            case 0x45: return PS2_SC_0;
            case 0x46: return PS2_SC_9;
            case 0x49: return PS2_SC_DOT;
            case 0x4A: return PS2_SC_SLASH;
            case 0x4B: return PS2_SC_L;
            case 0x4C: return PS2_SC_SEMICOLON;
            case 0x4D: return PS2_SC_P;
            case 0x4E: return PS2_SC_MINUS;
            case 0x52: return PS2_SC_APOSTROPHE;
            case 0x54: return PS2_SC_LBRACKET;
            case 0x55: return PS2_SC_EQUAL;
            case 0x58: return PS2_SC_CAPSLOCK;
            case 0x59: return PS2_SC_RIGHT_SHIFT;
            case 0x5A: return PS2_SC_ENTER;
            case 0x5B: return PS2_SC_RBRACKET;
            case 0x5D: return PS2_SC_BACKSLASH;
            case 0x66: return PS2_SC_BACKSPACE;
            case 0x69: return PS2_SC_KP_1;
            case 0x6B: return PS2_SC_KP_4;
            case 0x6C: return PS2_SC_KP_7;
            case 0x70: return PS2_SC_KP_0;
            case 0x71: return PS2_SC_KP_DOT;
            case 0x72: return PS2_SC_KP_2;
            case 0x73: return PS2_SC_KP_5;
            case 0x74: return PS2_SC_KP_6;
            case 0x75: return PS2_SC_KP_8;
            case 0x76: return PS2_SC_ESC;
            case 0x77: return PS2_SC_NUMLOCK;
            case 0x78: return PS2_SC_F11;
            case 0x79: return PS2_SC_KP_PLUS;
            case 0x7A: return PS2_SC_KP_3;
            case 0x7B: return PS2_SC_KP_MINUS;
            case 0x7C: return PS2_SC_KP_ASTERISK;
            case 0x7D: return PS2_SC_KP_9;
            case 0x7E: return PS2_SC_SCROLLLOCK;
            case 0x83: return PS2_SC_F7;
            default:   return PS2_SC_INVALID;
        }
    } else {
        switch (byte) {
            case 0x10: return PS2_SC_E0_WWW_SEARCH;
            case 0x11: return PS2_SC_E0_RIGHT_ALT;
            case 0x14: return PS2_SC_E0_RIGHT_CTRL;
            case 0x15: return PS2_SC_E0_PREV_TRACK;
            case 0x18: return PS2_SC_E0_WWW_FAV;
            case 0x1F: return PS2_SC_E0_LEFT_GUI;
            case 0x20: return PS2_SC_E0_WWW_REFRESH;
            case 0x21: return PS2_SC_E0_VOL_DOWN;
            case 0x23: return PS2_SC_E0_MUTE;
            case 0x27: return PS2_SC_E0_RIGHT_GUI;
            case 0x28: return PS2_SC_E0_WWW_STOP;
            case 0x2B: return PS2_SC_E0_CALCULATOR;
            case 0x2F: return PS2_SC_E0_APPS;
            case 0x30: return PS2_SC_E0_WWW_FORWARD;
            case 0x32: return PS2_SC_E0_VOL_UP;
            case 0x34: return PS2_SC_E0_PLAY_PAUSE;
            case 0x37: return PS2_SC_E0_POWER;
            case 0x38: return PS2_SC_E0_WWW_BACK;
            case 0x3A: return PS2_SC_E0_WWW_HOME;
            case 0x3B: return PS2_SC_E0_STOP;
            case 0x3F: return PS2_SC_E0_SLEEP;
            case 0x40: return PS2_SC_E0_MY_COMPUTER;
            case 0x48: return PS2_SC_E0_EMAIL;
            case 0x4A: return PS2_SC_E0_KP_SLASH;
            case 0x4D: return PS2_SC_E0_NEXT_TRACK;
            case 0x50: return PS2_SC_E0_MEDIA_SELECT;
            case 0x5A: return PS2_SC_E0_KP_ENTER;
            case 0x5E: return PS2_SC_E0_WAKE;
            case 0x69: return PS2_SC_E0_END;
            case 0x6B: return PS2_SC_E0_LEFT_ARROW;
            case 0x6C: return PS2_SC_E0_HOME;
            case 0x70: return PS2_SC_E0_INSERT;
            case 0x71: return PS2_SC_E0_DELETE;
            case 0x72: return PS2_SC_E0_DOWN_ARROW;
            case 0x74: return PS2_SC_E0_RIGHT_ARROW;
            case 0x75: return PS2_SC_E0_UP_ARROW;
            case 0x7A: return PS2_SC_E0_PAGE_DOWN;
            case 0x7D: return PS2_SC_E0_PAGE_UP;
            case 0x12: return PS2_SC_E0_PRINT_SCREEN;
            case 0x7C: return PS2_SC_E0_PRINT_SCREEN;
            default:   return PS2_SC_INVALID;
        }
    }
}

// PS/2 scancode → ASCII mapping for printable keys (0 for non-printable)
static inline char ps2_scancode_to_ascii(enum ps2_scancode sc) {
    switch (sc) {
        case PS2_SC_A: return 'a';
        case PS2_SC_B: return 'b';
        case PS2_SC_C: return 'c';
        case PS2_SC_D: return 'd';
        case PS2_SC_E: return 'e';
        case PS2_SC_F: return 'f';
        case PS2_SC_G: return 'g';
        case PS2_SC_H: return 'h';
        case PS2_SC_I: return 'i';
        case PS2_SC_J: return 'j';
        case PS2_SC_K: return 'k';
        case PS2_SC_L: return 'l';
        case PS2_SC_M: return 'm';
        case PS2_SC_N: return 'n';
        case PS2_SC_O: return 'o';
        case PS2_SC_P: return 'p';
        case PS2_SC_Q: return 'q';
        case PS2_SC_R: return 'r';
        case PS2_SC_S: return 's';
        case PS2_SC_T: return 't';
        case PS2_SC_U: return 'u';
        case PS2_SC_V: return 'v';
        case PS2_SC_W: return 'w';
        case PS2_SC_X: return 'x';
        case PS2_SC_Y: return 'y';
        case PS2_SC_Z: return 'z';
        case PS2_SC_1: return '1';
        case PS2_SC_2: return '2';
        case PS2_SC_3: return '3';
        case PS2_SC_4: return '4';
        case PS2_SC_5: return '5';
        case PS2_SC_6: return '6';
        case PS2_SC_7: return '7';
        case PS2_SC_8: return '8';
        case PS2_SC_9: return '9';
        case PS2_SC_0: return '0';
        case PS2_SC_SPACE:     return ' ';
        case PS2_SC_ENTER:     return '\r';
        case PS2_SC_TAB:       return '\t';
        case PS2_SC_BACKTICK:  return '`';
        case PS2_SC_MINUS:     return '-';
        case PS2_SC_EQUAL:     return '=';
        case PS2_SC_LBRACKET:  return '[';
        case PS2_SC_RBRACKET:  return ']';
        case PS2_SC_BACKSLASH: return '\\';
        case PS2_SC_SEMICOLON: return ';';
        case PS2_SC_APOSTROPHE:return '\'';
        case PS2_SC_COMMA:     return ',';
        case PS2_SC_DOT:       return '.';
        case PS2_SC_SLASH:     return '/';
        case PS2_SC_BACKSPACE: return '\b';
        default:               return '\0';
    }};
char ps2_scancode_to_uppercase(enum ps2_scancode sc);
int i8042_init(); 
#ifdef __cplusplus
}
#endif
static const char *kbd_pnpids[] = {"PNP0300",
    "PNP0301",
    "PNP0302",
    "PNP0303",
    "PNP0304",
    "PNP0305",
    "PNP0306",
    "PNP0307",
    "PNP0308",
    "PNP0309",
    "PNP030A",
    "PNP030B",
    "PNP0320",
    "PNP0321",
    "PNP0322",
    "PNP0323",
    "PNP0324",
    "PNP0325",
    "PNP0326",
    "PNP0327",
    "PNP0340",
    "PNP0341",
    "PNP0342",
    "PNP0343",
    "PNP0343",
    "PNP0344",
NULL,};

static const char *mouse_pnpids[] = {"PNP0F00",
	"PNP0F01",
	"PNP0F02",
	"PNP0F03",
	"PNP0F04",
	"PNP0F05",
	"PNP0F06",
	"PNP0F07",
	"PNP0F08",
	"PNP0F09",
	"PNP0F0A",
	"PNP0F0B",
	"PNP0F0C",
	"PNP0F0D",
	"PNP0F0E",
	"PNP0F0F",
	"PNP0F10",
	"PNP0F11",
	"PNP0F12",
	"PNP0F13",
	"PNP0F14",
	"PNP0F15",
	"PNP0F16",
	"PNP0F17",
	"PNP0F18",
	"PNP0F19",
	"PNP0F1A",
	"PNP0F1B",
	"PNP0F1C",
	"PNP0F1D",
	"PNP0F1E",
	"PNP0F1F",
	"PNP0F20",
	"PNP0F21",
	"PNP0F22",
	"PNP0F23",
	"PNP0FFC",
	"PNP0FFF",
NULL};
enum PS2DEVICETYPE {
    KEYBOARD,
    MOUSE,
    UNKNOWN
};
struct PS2PORT {
    enum PS2DEVICETYPE type;
    bool usable;
};
struct ps2controller {
    bool inited;
    struct PS2PORT port1;
    struct PS2PORT port2;
};