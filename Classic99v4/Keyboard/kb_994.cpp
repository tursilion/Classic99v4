// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class sets up the 99/4 keyboard

#include "kb_994.h"

static Array8x8 KEYS = {  
    // Keyboard - 99/4
{    /* unused */	KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE },

{    /* 1 */		KEY_N, KEY_H, KEY_U, KEY_SEVEN, KEY_C, KEY_D, KEY_R, KEY_FOUR },
{    /* Joy 1 */	KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE },
{    /* 3 */		KEY_PERIOD, KEY_K, KEY_O, KEY_NINE, KEY_Z, KEY_A, KEY_W, KEY_TWO },

{    /* Joy 2 */	KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE, KEY_ESCAPE },
	
{    /* 5 */		KEY_M, KEY_J, KEY_I, KEY_EIGHT, KEY_X, KEY_S, KEY_E, KEY_THREE },
{    /* 6 */		KEY_B, KEY_G, KEY_Y, KEY_SIX, KEY_V, KEY_F, KEY_T, KEY_FIVE },
{    /* 7 */		KEY_ENTER, KEY_L, KEY_P, KEY_ZERO, KEY_LEFT_SHIFT, KEY_SPACE, KEY_Q, KEY_ONE }
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
