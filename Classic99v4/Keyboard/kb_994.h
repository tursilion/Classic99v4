// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the scan matrix of the 99/4 keyboard
// TODO: add the joysticks as well

// TODO: slow down hacks for overdrive are needed eventually

#ifndef KB994_H
#define KB994_H

#include "..\EmulatorSupport\peripheral.h"
#include "TIKeyboard.h"

// CPU ROM memory
class KB994 : public TIKeyboard {
public:
    KB994(Classic99System *core);
    virtual ~KB994();

    // ========== TIKeyboard interface ============
    int getJoy1Col() override;
    int getJoy2Col() override;
    Array8x8 &getKeyArray() override; 
};


#endif
