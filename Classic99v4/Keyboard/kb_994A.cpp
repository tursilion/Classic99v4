// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class sets up the 99/4((A)) keyboard

#include <raylib.h>
#include "kb_994A.h"

static Array8x8 KEYS = {  
	// Keyboard - 99/4A
{	/* Joy 2 */		KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE },

{	/* 1 */			KEY_M, KEY_J, KEY_U, KEY_SEVEN, KEY_FOUR, KEY_F, KEY_R, KEY_V },			// MJU7 4FRV
{	/* 2 */			KEY_SLASH, KEY_SEMICOLON, KEY_P, KEY_ZERO, KEY_ONE, KEY_A, KEY_Q, KEY_Z },	// /;P0 1AQZ
{	/* 3 */			KEY_PERIOD, KEY_L, KEY_O, KEY_NINE, KEY_TWO, KEY_S, KEY_W, KEY_X },			// .LO9 2SWX

{	/* Joy 1 */		KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE },
	
{	/* 5 */			KEY_COMMA, KEY_K, KEY_I, KEY_EIGHT, KEY_THREE, KEY_D, KEY_E, KEY_C },				// ,KI8 3DEC
{	/* 6 */			KEY_N, KEY_H, KEY_Y, KEY_SIX, KEY_FIVE, KEY_G, KEY_T, KEY_B },					// NHY6 5GTB
{	/* 7 */			KEY_EQUAL, KEY_SPACE, KEY_ENTER, KEY_ESCAPE, KEY_LEFT_ALT, KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_ESCAPE }
	 																		// = rx fscx
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

Array8x8 &KB994A::getKeyArray() {
    return KEYS;
}
