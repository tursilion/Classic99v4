// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class sets up the 99/4 keyboard

#include "kb_994.h"

static const Array8x8 KEYS = {  
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
{    /* Joy 1 */	3*KWSIZE+ 0, 1*KWSIZE+ 0, 1*KWSIZE+ 2, 2*KWSIZE+ 1, 0*KWSIZE+ 1, 0, 0, 0 },                     // FLRDU
{    /* 3 */		3*KWSIZE+23, 2*KWSIZE+22, 1*KWSIZE+21, 0*KWSIZE+22, 3*KWSIZE+19, 2*KWSIZE+18, 1*KWSIZE+ 9, 0*KWSIZE+ 8 },   // .KI9 ZAW2

{    /* Joy 2 */	3*KWSIZE+31, 1*KWSIZE+31, 1*KWSIZE+33, 2*KWSIZE+32, 0*KWSIZE+32, 0, 0, 0 },                     // FLRDU
	
{    /* 5 */		3*KWSIZE+21, 2*KWSIZE+20, 1*KWSIZE+21, 0*KWSIZE+20, 3*KWSIZE+11, 2*KWSIZE+ 9, 1*KWSIZE+11, 0*KWSIZE+10 },   // MJI8 XSE3
{    /* 6 */		3*KWSIZE+17, 2*KWSIZE+16, 1*KWSIZE+17, 0*KWSIZE+16, 3*KWSIZE+15, 2*KWSIZE+14, 1*KWSIZE+15, 0*KWSIZE+14 },   // BGY6 VFT5
{    /* 7 */		3*KWSIZE+25, 2*KWSIZE+24, 1*KWSIZE+25, 0*KWSIZE+24, 3*KWSIZE+ 6, 2*KWSIZE+ 6, 1*KWSIZE+ 6, 0*KWSIZE+ 6 }    // rLP0 s Q1
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
    return KEYS;
}

const Array8x8 &KB994::getKeyDebugArray() {
    return keyDebugMap4;
}

const char *KB994::getKeyDebugString() {
    return ti4Key;
}

bool KB994::is4A() {
    return false;
}
