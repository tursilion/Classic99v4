// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class sets up the 99/4 keyboard

#include "kb_994.h"

static const Array8x8 keys4 = {  
    // Keyboard - 99/4
{    /* unused */	RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE },

{    /* 1 */		RL_KEY_N, RL_KEY_H, RL_KEY_U, RL_KEY_SEVEN, RL_KEY_C, RL_KEY_D, RL_KEY_R, RL_KEY_FOUR },
{    /* Joy 1 */	RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE },
{    /* 3 */		RL_KEY_PERIOD, RL_KEY_K, RL_KEY_O, RL_KEY_NINE, RL_KEY_Z, RL_KEY_A, RL_KEY_W, RL_KEY_TWO },

{    /* Joy 2 */	RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE },
	
{    /* 5 */		RL_KEY_M, RL_KEY_J, RL_KEY_I, RL_KEY_EIGHT, RL_KEY_X, RL_KEY_S, RL_KEY_E, RL_KEY_THREE },
{    /* 6 */		RL_KEY_B, RL_KEY_G, RL_KEY_Y, RL_KEY_SIX, RL_KEY_V, RL_KEY_F, RL_KEY_T, RL_KEY_FIVE },
{    /* 7 */		RL_KEY_ENTER, RL_KEY_L, RL_KEY_P, RL_KEY_ZERO, RL_KEY_LEFT_SHIFT, RL_KEY_SPACE, RL_KEY_Q, RL_KEY_ONE }
};

//             1         2         3
//   01234567890123456789012345678901234
static const char *ti4Key =
//             1         2         3
//   01234567890123456789012345678901234
    "  ^    1 2 3 4 5 6 7 8 9 0       ^ \0"  //0
    " < >    Q W E R T Y U I O P     < >\0"  //1
    "  v    s A S D F G H J K L       v \0"  //2
    " ---    ^ Z X C V B N M . <     ---\0"  //3
    "            - SPACE -              ";   //4

static const Array8x8 keyDebugMap4 = {  
    // Keyboard - 99/4
{    /* unused */	0,0,0,0, 0,0,0,0 },

{    /* 1 */		3*KWSIZE+19, 2*KWSIZE+18, 1*KWSIZE+19, 0*KWSIZE+18, 3*KWSIZE+13, 2*KWSIZE+12, 1*KWSIZE+13, 0*KWSIZE+12},    // NHU7 CDR4
{    /* Joy 1 */	3*KWSIZE+ 0, 1*KWSIZE+ 0, 1*KWSIZE+ 2, 2*KWSIZE+ 1, 0*KWSIZE+ 1, 0, 0, 0 },                                 // FLRDU
{    /* 3 */		3*KWSIZE+23, 2*KWSIZE+22, 1*KWSIZE+23, 0*KWSIZE+22, 3*KWSIZE+ 9, 2*KWSIZE+ 8, 1*KWSIZE+ 9, 0*KWSIZE+ 8 },   // .KO9 ZAW2

{    /* Joy 2 */	3*KWSIZE+31, 1*KWSIZE+31, 1*KWSIZE+33, 2*KWSIZE+32, 0*KWSIZE+32, 0, 0, 0 },                                 // FLRDU
	
{    /* 5 */		3*KWSIZE+21, 2*KWSIZE+20, 1*KWSIZE+21, 0*KWSIZE+20, 3*KWSIZE+11, 2*KWSIZE+10, 1*KWSIZE+11, 0*KWSIZE+10 },   // MJI8 XSE3
{    /* 6 */		3*KWSIZE+17, 2*KWSIZE+16, 1*KWSIZE+17, 0*KWSIZE+16, 3*KWSIZE+15, 2*KWSIZE+14, 1*KWSIZE+15, 0*KWSIZE+14 },   // BGY6 VFT5
{    /* 7 */		3*KWSIZE+25, 2*KWSIZE+24, 1*KWSIZE+25, 0*KWSIZE+24, 3*KWSIZE+ 7, 2*KWSIZE+ 6, 1*KWSIZE+ 7, 0*KWSIZE+ 6 }    // rLP0 s Q1
};

// TODO: This is the 4A map, not the 4 map!!
// This maps the ASCII set to the two keys to press - 0 for no key
// 32-126
static const int asciiMap4[] = {
// ascii    key             meta
/*  ' ', */ RL_KEY_SPACE,   0,
/*  '!', */ RL_KEY_ONE,     RL_KEY_LEFT_SHIFT,
/*  '\"',*/ RL_KEY_P,       RL_KEY_LEFT_ALT,
/*  '#', */ RL_KEY_THREE,   RL_KEY_LEFT_SHIFT,
/*  '$', */ RL_KEY_FOUR,    RL_KEY_LEFT_SHIFT,
/*  '&', */ RL_KEY_SEVEN,   RL_KEY_LEFT_SHIFT,
/*  '%', */ RL_KEY_FIVE,    RL_KEY_LEFT_SHIFT,
/*  '\'',*/ RL_KEY_O,       RL_KEY_LEFT_ALT,
/*  '(', */ RL_KEY_NINE,    RL_KEY_LEFT_SHIFT,
/*  ')', */ RL_KEY_ZERO,    RL_KEY_LEFT_SHIFT,
/*  '*', */ RL_KEY_EIGHT,   RL_KEY_LEFT_SHIFT,
/*  '+', */ RL_KEY_EQUAL,   RL_KEY_LEFT_SHIFT,
/*  ',', */ RL_KEY_COMMA,   0,
/*  '-', */ RL_KEY_SLASH,   RL_KEY_LEFT_SHIFT,
/*  '.', */ RL_KEY_PERIOD,  0,
/*  '/', */ RL_KEY_SLASH,   0,
/*  '0', */ RL_KEY_ZERO,    0,
/*  '1', */ RL_KEY_ONE,     0,
/*  '2', */ RL_KEY_TWO,     0,
/*  '3', */ RL_KEY_THREE,   0,
/*  '4', */ RL_KEY_FOUR,    0,
/*  '5', */ RL_KEY_FIVE,    0,
/*  '6', */ RL_KEY_SIX,     0,
/*  '7', */ RL_KEY_SEVEN,   0,
/*  '8', */ RL_KEY_EIGHT,   0,
/*  '9', */ RL_KEY_NINE,    0,
/*  ':', */ RL_KEY_SEMICOLON, RL_KEY_LEFT_SHIFT,
/*  ';', */ RL_KEY_SEMICOLON, 0,
/*  '<', */ RL_KEY_COMMA,   RL_KEY_LEFT_SHIFT,
/*  '=', */ RL_KEY_EQUAL,   0,
/*  '>', */ RL_KEY_PERIOD,  0,
/*  '?', */ RL_KEY_I,       RL_KEY_LEFT_ALT,
/*  '@', */ RL_KEY_TWO,     RL_KEY_LEFT_SHIFT,
/*  'A', */ RL_KEY_A,       RL_KEY_LEFT_SHIFT,
/*  'B', */ RL_KEY_B,       RL_KEY_LEFT_SHIFT,
/*  'C', */ RL_KEY_C,       RL_KEY_LEFT_SHIFT,
/*  'D', */ RL_KEY_D,       RL_KEY_LEFT_SHIFT,
/*  'E', */ RL_KEY_E,       RL_KEY_LEFT_SHIFT,
/*  'F', */ RL_KEY_F,       RL_KEY_LEFT_SHIFT,
/*  'G', */ RL_KEY_G,       RL_KEY_LEFT_SHIFT,
/*  'H', */ RL_KEY_H,       RL_KEY_LEFT_SHIFT,
/*  'I', */ RL_KEY_I,       RL_KEY_LEFT_SHIFT,
/*  'J', */ RL_KEY_J,       RL_KEY_LEFT_SHIFT,
/*  'K', */ RL_KEY_K,       RL_KEY_LEFT_SHIFT,
/*  'L', */ RL_KEY_L,       RL_KEY_LEFT_SHIFT,
/*  'M', */ RL_KEY_M,       RL_KEY_LEFT_SHIFT,
/*  'N', */ RL_KEY_N,       RL_KEY_LEFT_SHIFT,
/*  'O', */ RL_KEY_O,       RL_KEY_LEFT_SHIFT,
/*  'P', */ RL_KEY_P,       RL_KEY_LEFT_SHIFT,
/*  'Q', */ RL_KEY_Q,       RL_KEY_LEFT_SHIFT,
/*  'R', */ RL_KEY_R,       RL_KEY_LEFT_SHIFT,
/*  'S', */ RL_KEY_S,       RL_KEY_LEFT_SHIFT,
/*  'T', */ RL_KEY_T,       RL_KEY_LEFT_SHIFT,
/*  'U', */ RL_KEY_U,       RL_KEY_LEFT_SHIFT,
/*  'V', */ RL_KEY_V,       RL_KEY_LEFT_SHIFT,
/*  'W', */ RL_KEY_W,       RL_KEY_LEFT_SHIFT,
/*  'X', */ RL_KEY_X,       RL_KEY_LEFT_SHIFT,
/*  'Y', */ RL_KEY_Y,       RL_KEY_LEFT_SHIFT,
/*  'Z', */ RL_KEY_Z,       RL_KEY_LEFT_SHIFT,
/*  '[', */ RL_KEY_R,       RL_KEY_LEFT_ALT,
/*  '\\',*/ RL_KEY_Z,       RL_KEY_LEFT_ALT,
/*  ']', */ RL_KEY_T,       RL_KEY_LEFT_ALT,
/*  '^', */ RL_KEY_SIX,     RL_KEY_LEFT_SHIFT,
/*  '_', */ RL_KEY_U,       RL_KEY_LEFT_ALT,
/*  '`', */ RL_KEY_C,       RL_KEY_LEFT_ALT,
/*  'a', */ RL_KEY_A,       0,
/*  'b', */ RL_KEY_B,       0,
/*  'c', */ RL_KEY_C,       0,
/*  'd', */ RL_KEY_D,       0,
/*  'e', */ RL_KEY_E,       0,
/*  'f', */ RL_KEY_F,       0,
/*  'g', */ RL_KEY_G,       0,
/*  'h', */ RL_KEY_H,       0,
/*  'i', */ RL_KEY_I,       0,
/*  'j', */ RL_KEY_J,       0,
/*  'k', */ RL_KEY_K,       0,
/*  'l', */ RL_KEY_L,       0,
/*  'm', */ RL_KEY_M,       0,
/*  'n', */ RL_KEY_N,       0,
/*  'o', */ RL_KEY_O,       0,
/*  'p', */ RL_KEY_P,       0,
/*  'q', */ RL_KEY_Q,       0,
/*  'r', */ RL_KEY_R,       0,
/*  's', */ RL_KEY_S,       0,
/*  't', */ RL_KEY_T,       0,
/*  'u', */ RL_KEY_U,       0,
/*  'v', */ RL_KEY_V,       0,
/*  'w', */ RL_KEY_W,       0,
/*  'x', */ RL_KEY_X,       0,
/*  'y', */ RL_KEY_Y,       0,
/*  'z', */ RL_KEY_Z,       0,
/*  '{', */ RL_KEY_F,       RL_KEY_LEFT_ALT,
/*  '|', */ RL_KEY_A,       RL_KEY_LEFT_ALT,
/*  '}', */ RL_KEY_G,       RL_KEY_LEFT_ALT,
/*  '~', */ RL_KEY_W,       RL_KEY_LEFT_ALT
};

// not much needed for construction
KB994::KB994(Classic99System *core) 
    : TIKeyboard(core)
{
}

KB994::~KB994() {
}

int KB994::getJoy1Col() {
    return 2;
}

int KB994::getJoy2Col() {
    return 4;
}

const Array8x8 &KB994::getKeyArray() {
    return keys4;
}

const Array8x8 &KB994::getKeyDebugArray() {
    return keyDebugMap4;
}

const int *KB994::getAsciiMap() {
    return asciiMap4;
}

const char *KB994::getKeyDebugString() {
    return ti4Key;
}

bool KB994::is4A() {
    return false;
}
