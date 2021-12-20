// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the 99/4 keyboard

// TODO: we need a way to implement the 9901's control bit (bit 0), since that
// changes the meaning of the bits. Maybe we merge that much of the 9901 into
// the 9900's emulation?

// The CRU bits >0000 through >001f are repeated through the whole range!
// from >0000 to >07FF

#if 0

BIT	HW	C99	Purpose						Status
-------------------						----------------------
3	1	0	Clock, keyboard enter, fire	Implemented, but wrong?
4	1	0	Keyboard l, left			Implemented, but wrong?
5	1	1	Keyboard p, right			Implemented
6	1	1	keyboard 0, down			Implemented
7	1	1	keyboard shift, up, alpha	Implemented
8	1	1	keyboard space				Implemented
9	1	0	keyboard q					Implemented, but wrong?
10	1	1	keyboard l					Implemented

18	0	0	keyboard select bit 2		Implemented as loopback
19	0	0	keyboard select bit 1		Implemented as loopback
20	0	0	keyboard select bit 0		Implemented as loopback

21	1	0	alpha lock					Implemented, but wrong?

#endif

#include "kb_994.h"

#define JOY1COL 2
#define JOY2COL 4

int KEYS[8][8]= {  
    // Keyboard - 99/4
    /* unused */	ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE,

    /* 1 */			ALLEGRO_KEY_N, ALLEGRO_KEY_H, ALLEGRO_KEY_U, ALLEGRO_KEY_7, ALLEGRO_KEY_C, ALLEGRO_KEY_D, ALLEGRO_KEY_R, ALLEGRO_KEY_4,
    /* Joy 1 */		ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE,
    /* 3 */			ALLEGRO_KEY_FULLSTOP, ALLEGRO_KEY_K, ALLEGRO_KEY_O, ALLEGRO_KEY_9, ALLEGRO_KEY_Z, ALLEGRO_KEY_A, ALLEGRO_KEY_W, ALLEGRO_KEY_2,

    /* Joy 2 */		ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE, ALLEGRO_KEY_ESCAPE,
	
    /* 5 */			ALLEGRO_KEY_M, ALLEGRO_KEY_J, ALLEGRO_KEY_I, ALLEGRO_KEY_8, ALLEGRO_KEY_X, ALLEGRO_KEY_S, ALLEGRO_KEY_E, ALLEGRO_KEY_3,
    /* 6 */			ALLEGRO_KEY_B, ALLEGRO_KEY_G, ALLEGRO_KEY_Y, ALLEGRO_KEY_6, ALLEGRO_KEY_V, ALLEGRO_KEY_F, ALLEGRO_KEY_T, ALLEGRO_KEY_5,
    /* 7 */			ALLEGRO_KEY_ENTER, ALLEGRO_KEY_L, ALLEGRO_KEY_P, ALLEGRO_KEY_0, ALLEGRO_KEY_LSHIFT, ALLEGRO_KEY_SPACE, ALLEGRO_KEY_Q, ALLEGRO_KEY_1
};

// not much needed for construction
KB994::KB994(Classic99System *core) 
    : Classic99Peripheral(core)
	, bJoy(true)
	, fJoystickActiveOnKeys(0)
    , scanCol(0)
    , alphaActive(false)
{
}
KB994::~KB994() {
}

uint8_t KB994::CheckJoysticks(int addr, int scanCol, ALLEGRO_KEYBOARD_STATE *pState) {
	int joyX, joyY, joyFire;
	int ret=1;

	// Read external hardware
	joyX=0;
	joyY=0;
	joyFire=0;

	if ((scanCol == JOY1COL) || (scanCol == JOY2COL))				// reading joystick
	{	
	    // TODO: add support for real joysticks...
		if (bJoy) {
			int device = -1;
			int index = 0;

			// allow for joystick number 1-16
			if (scanCol == JOY2COL) {
				index = 1;	// else it's joystick 1, so index 0
			}

//			if ((joyStick[index].mode > 0)&&(joyStick[index].mode <= 16)) {
//				device = (joyStick[index].mode-1)+JOYSTICKID1;
//			}

			if (device!=-1) {
#if 0
				// check if we previously disabled this stick
				if ((installedJoysticks&(1<<(device-JOYSTICKID1))) == 0) {
					return 1;
				}

				memset(&myJoy, 0, sizeof(myJoy));
				myJoy.dwSize=sizeof(myJoy);
				myJoy.dwFlags=JOY_RETURNALL | JOY_USEDEADZONE;
				MMRESULT joyret = joyGetPosEx(device, &myJoy);
				if (JOYERR_NOERROR == joyret) {
					// TODO: we do all this work to calculate both axes plus the buttons, but we only
					// actually want one direction OR the button, so we should make the joystick read smarter, cache
					// myJoy for a frame or so and just pull out the part we actually need...

					if (0!=(myJoy.dwButtons & joyStick[index].btnMask)) {
						joyFire=1;
					}

					unsigned int half = (joyStick[index].maxXDead-joyStick[index].minXDead)/2;
					unsigned int axis;
					switch(joyStick[index].Xaxis) {
						case 0: axis = myJoy.dwXpos; break;
						case 1: axis = myJoy.dwYpos; break;
						case 2: axis = myJoy.dwZpos; break;
						case 3: axis = myJoy.dwRpos; break;
						case 4: axis = myJoy.dwUpos; break;
						case 5: axis = myJoy.dwVpos; break;
						case 6: 
							if (myJoy.dwPOV <= 35999) {
								// use sin() (x component)			 degrees*100 to radians
								axis = (unsigned int)(sin(myJoy.dwPOV * 0.000174533)*(half*2)+half+joyStick[index].minXDead);
							} else {
								axis = joyStick[index].minXDead+half;
							}
							break;
						case 7: 
							if (myJoy.dwPOV <= 35999) {
								// use cos() (y component)			 degrees*100 to radians
								axis = (unsigned int)(cos(myJoy.dwPOV * 0.000174533)*(half*2)+half+joyStick[index].minXDead);
							} else {
								axis = joyStick[index].minXDead+half;
							}
							break;

						default: axis = joyStick[index].minXDead+half;
					}
					if (axis<joyStick[index].minXDead) {
						joyX=-4;
					} else if (axis>joyStick[index].maxXDead) {
						joyX=4;
					}

					half = (joyStick[index].maxYDead-joyStick[index].minYDead)/2;
					switch(joyStick[index].Yaxis) {
						case 0: axis = myJoy.dwXpos; break;
						case 1: axis = myJoy.dwYpos; break;
						case 2: axis = myJoy.dwZpos; break;
						case 3: axis = myJoy.dwRpos; break;
						case 4: axis = myJoy.dwUpos; break;
						case 5: axis = myJoy.dwVpos; break;
						case 6: 
							if (myJoy.dwPOV <= 35999) {
								// use sin() (x component)			 degrees*100 to radians, inverted for y
								axis = (unsigned int)((-sin(myJoy.dwPOV * 0.000174533))*(half*2)+half+joyStick[index].minYDead);
							} else {
								axis = joyStick[index].minYDead+half;
							}
							break;
						case 7:
							if (myJoy.dwPOV <= 35999) {
								// use cos() (y component)			 degrees*100 to radians, inverted for y
								axis = (unsigned int)((-cos(myJoy.dwPOV * 0.000174533))*(half*2)+half+joyStick[index].minYDead);
							} else {
								axis = joyStick[index].minYDead+half;
							}
							break;
						default: axis = joyStick[index].minYDead+half;
					}
					if (axis<joyStick[index].minYDead) {
						joyY=4;
					} else if (axis>joyStick[index].maxYDead) {
						joyY=-4;
					}
				} else {
					// disable this joystick so we don't slow to a crawl
					// trying to access it. We'll check again on a reset
					debug_write("Disabling joystick %d - error %d reading it.", (device-JOYSTICKID1)+1, joyret);
					installedJoysticks&=~(1<<(device-JOYSTICKID1));
				}
#endif
			} else {	// read the keyboard
				// if just activating the joystick, so make sure there's no fctn-arrow keys active
				// just forcibly turn them off! Should only need to do this once
				// TODO: decode is only needed for my PS/2 emulation

				if (al_key_down(pState, ALLEGRO_KEY_TAB)) {
					joyFire=1;
					//if (0 == fJoystickActiveOnKeys) {
					//	decode(0xf0);	// key up
					//	decode(VK_TAB);
					//}
				}
				if (al_key_down(pState, ALLEGRO_KEY_LEFT)) {
					joyX=-4;
					//if (0 == fJoystickActiveOnKeys) {
					//	decode(0xe0);	// extended
					//	decode(0xf0);	// key up
					//	decode(VK_LEFT);
					//}
				}
				if (al_key_down(pState, ALLEGRO_KEY_RIGHT)) {
					joyX=4;
					//if (0 == fJoystickActiveOnKeys) {
					//	decode(0xe0);	// extended
					//	decode(0xf0);	// key up
					//	decode(VK_RIGHT);
					//}
				}
				if (al_key_down(pState, ALLEGRO_KEY_UP)) {
					joyY=4;
					//if (0 == fJoystickActiveOnKeys) {
					//	decode(0xe0);	// extended
					//	decode(0xf0);	// key up
					//	decode(VK_UP);
					//}
				}
				if (al_key_down(pState, ALLEGRO_KEY_DOWN)) {
					joyY=-4;
					//if (0 == fJoystickActiveOnKeys) {
					//	decode(0xe0);	// extended
					//	decode(0xf0);	// key up
					//	decode(VK_DOWN);
					//}
				}

				// TODO: we can remove this here eventually...
				fJoystickActiveOnKeys=180;		// frame countdown! Don't use PS2 arrow keys for this many frames
			}
		}

		// TODO: this is where we actually extract the data we actually care about...
		if (addr == 3) {	
			// fire button...
			if (joyFire)	// button reads normally
			{
				ret=0;
			}
		} else {
			// directions...
			switch (addr-3) {
				case 1: if (joyX ==-4) ret=0; break;
				case 2: if (joyX == 4) ret=0; break;
				case 3: if (joyY ==-4) ret=0; break;
				case 4: if (joyY == 4) ret=0; break;
			}
		}
	}
	return ret;
}

uint8_t KB994::read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) {
    // there are no extra cycles used by the TI's CRU
    (void)isIO;
    (void)cycles;
    (void)rmw;

    ALLEGRO_KEYBOARD_STATE state;
    al_get_keyboard_state(&state);

    // the address will match the CRU bits as above
    
    // First check if we are reading alpha lock - we do the inversion of caps lock
    if ((addr == 0x07) && (alphaActive)) {
        uint8_t ret = 0;
        // TODO: verify this works as CAPS /state/, not caps KEY status
        if (al_key_down(&state, ALLEGRO_KEY_CAPSLOCK)) {
            ret = 1;
        }
        return 1;
    }

    // try joysticks... 
    uint8_t ret = CheckJoysticks(addr, scanCol, &state);
    if (1 == ret) {
        // nothing else matched, so check the keyboard array
        if (al_key_down(&state, KEYS[scanCol][addr-3])) {
            ret = 0;
        }
    }

    return ret;
}

void KB994::write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) {
    // there are no extra cycles used by the TI's CRU
    (void)isIO;
    (void)cycles;
    (void)rmw;

    // only 4 bits matter here
    switch (addr) {
        case 0x12:
            scanCol = (scanCol&0x3) | (data?0:4);
            break;

        case 0x13:
            scanCol = (scanCol&0x5) | (data?0:2);
            break;

        case 0x14:
            scanCol = (scanCol&0x6) | (data?0:1);
            break;

        case 0x21:
            if (data) {
                alphaActive = false;
            } else {
                alphaActive = true;
            }
            break;
    }
}

bool KB994::init(int idx) {
	al_install_joystick();
	al_install_keyboard();
    return true;
}

bool KB994::operate(double timestamp) {
    // nothing special to do here
    return true;
}

bool KB994::cleanup() {
    al_uninstall_keyboard();
	al_uninstall_joystick();
    return true;
}

void KB994::getDebugSize(int &x, int &y) {
    // we had a nice bitmapped keyboard map, but right now, not sure
    x=0;
    y=0;
}

void KB994::getDebugWindow(char *buffer) {
    // nothing at the moment
}

int KB994::saveStateSize() {
    return 4;
}

bool KB994::saveState(unsigned char *buffer) {
	// only using 2 of the 4 bytes...
	buffer[0] = scanCol&0xff;
	buffer[1] = alphaActive ? 1:0;
    return true;
}

bool KB994::restoreState(unsigned char *buffer) {
	scanCol = buffer[0];
	alphaActive = buffer[1] ? true : false;
	return true;
}
