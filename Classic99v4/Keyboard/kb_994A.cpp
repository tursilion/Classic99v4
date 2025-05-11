// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class sets up the 99/4((A)) keyboard

#include <raylib.h>
#include "kb_994A.h"

static Array8x8 KEYS = {  
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
