// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class sets up the 99/4((A)) keyboard

#include <raylib.h>
#include "kb_994A.h"

static const Array8x8 KEYS = {  
	// Keyboard - 99/4A
{	/* Joy 2 */		RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE },

{	/* 1 */			RL_KEY_M, RL_KEY_J, RL_KEY_U, RL_KEY_SEVEN, RL_KEY_FOUR, RL_KEY_F, RL_KEY_R, RL_KEY_V },			// MJU7 4FRV
{	/* 2 */			RL_KEY_SLASH, RL_KEY_SEMICOLON, RL_KEY_P, RL_KEY_ZERO, RL_KEY_ONE, RL_KEY_A, RL_KEY_Q, RL_KEY_Z },	// /;P0 1AQZ
{	/* 3 */			RL_KEY_PERIOD, RL_KEY_L, RL_KEY_O, RL_KEY_NINE, RL_KEY_TWO, RL_KEY_S, RL_KEY_W, RL_KEY_X },			// .LO9 2SWX

{	/* Joy 1 */		RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE, RL_KEY_ESCAPE },
	
{	/* 5 */			RL_KEY_COMMA, RL_KEY_K, RL_KEY_I, RL_KEY_EIGHT, RL_KEY_THREE, RL_KEY_D, RL_KEY_E, RL_KEY_C },				// ,KI8 3DEC
{	/* 6 */			RL_KEY_N, RL_KEY_H, RL_KEY_Y, RL_KEY_SIX, RL_KEY_FIVE, RL_KEY_G, RL_KEY_T, RL_KEY_B },					// NHY6 5GTB
{	/* 7 */			RL_KEY_EQUAL, RL_KEY_SPACE, RL_KEY_ENTER, RL_KEY_ESCAPE, RL_KEY_LEFT_ALT, RL_KEY_LEFT_SHIFT, RL_KEY_LEFT_CONTROL, RL_KEY_ESCAPE }
	 																		// = rx fscx
};

// text maps for the keys - every key has a space before it to allow for a code to be inserted
static const char *ti4aKey =
//             1         2         3
//   01234567890123456789012345678901234
    "  ^    1 2 3 4 5 6 7 8 9 0 =     ^ \0"  //0
    " < >    Q W E R T Y U I O P /   < >\0"  //1
    "  v      A S D F G H J K L ; <   v \0"  //2
    " ---    ^ Z X C V B N M , . ^   ---\0"  //3
    "        ^ c ---- SPACE ---- f      ";   //4

// these maps map the KEYS 8x8 array to byte offsets in the debug view
static const Array8x8 keyDebugMap4a = {
{	/* Joy 2 */		3*KWSIZE+31, 1*KWSIZE+31, 1*KWSIZE+33, 2*KWSIZE+32, 0*KWSIZE+32, 0, 0, 0 },                     // FLRDU

{	/* 1 */			3*KWSIZE+21, 2*KWSIZE+20, 1*KWSIZE+19, 0*KWSIZE+18, 0*KWSIZE+12, 2*KWSIZE+14, 1*KWSIZE+13, 3*KWSIZE+15 },   // MJU7 4FRV
{	/* 2 */			1*KWSIZE+27, 2*KWSIZE+26, 1*KWSIZE+25, 0*KWSIZE+24, 0*KWSIZE+ 6, 2*KWSIZE+ 8, 1*KWSIZE+ 7, 3*KWSIZE+ 9 },	// /;P0 1AQZ
{	/* 3 */			3*KWSIZE+25, 2*KWSIZE+24, 1*KWSIZE+23, 0*KWSIZE+22, 0*KWSIZE+ 8, 2*KWSIZE+10, 1*KWSIZE+ 9, 3*KWSIZE+11 },	// .LO9 2SWX

{	/* Joy 1 */		3*KWSIZE+ 0, 1*KWSIZE+ 0, 1*KWSIZE+ 2, 2*KWSIZE+ 1, 0*KWSIZE+ 1, 0, 0, 0  },                    // FLRDU
	
{	/* 5 */			3*KWSIZE+23, 2*KWSIZE+22, 1*KWSIZE+21, 0*KWSIZE+20, 0*KWSIZE+10, 2*KWSIZE+12, 1*KWSIZE+11, 3*KWSIZE+13 },	// ,KI8 3DEC
{	/* 6 */			3*KWSIZE+19, 2*KWSIZE+18, 1*KWSIZE+17, 0*KWSIZE+16, 0*KWSIZE+14, 2*KWSIZE+16, 1*KWSIZE+15, 3*KWSIZE+17 },	// NHY6 5GTB
{	/* 7 */			0*KWSIZE+26, 4*KWSIZE+16, 2*KWSIZE+28, 0,           4*KWSIZE+27, 3*KWSIZE+ 7, 4*KWSIZE+ 9, 0 }          // = rx fscx
};

// not much needed for construction
KB994A::KB994A(Classic99System *core) 
    : TIKeyboard(core)
{
}
KB994A::~KB994A() {
}

int KB994A::getJoy1Col() {
    return 4;
}

int KB994A::getJoy2Col() {
    return 0;
}

const Array8x8 &KB994A::getKeyArray() {
    return KEYS;
}

const Array8x8 &KB994A::getKeyDebugArray() {
    return keyDebugMap4a;
}

const char *KB994A::getKeyDebugString() {
    return ti4aKey;
}

bool KB994A::is4A() {
    return true;
}
