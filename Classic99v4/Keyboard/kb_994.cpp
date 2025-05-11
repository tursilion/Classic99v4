// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class sets up the 99/4 keyboard

#include "kb_994.h"

static Array8x8 KEYS = {  
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

Array8x8 &KB994::getKeyArray() {
    return KEYS;
}
