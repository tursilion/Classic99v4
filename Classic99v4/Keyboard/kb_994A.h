// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the scan matrix of the 99/4 keyboard
// TODO: add the joysticks as well

// TODO: slow down hacks for overdrive are needed eventually

#ifndef KB994_H
#define KB994_H

#include "../EmulatorSupport/peripheral.h"
#include "TIKeyboard.h"

// CPU ROM memory
class KB994A : public TIKeyboard {
public:
    KB994A(Classic99System *core);
    virtual ~KB994A();

    // ========== TIKeyboard interface ============
    int getJoy1Col() override;
    int getJoy2Col() override;
    const Array8x8 &getKeyArray() override; 
    const Array8x8 &getKeyDebugArray() override;
    const char *getKeyDebugString() override;
    bool is4A() override;

};


#endif
