// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the ROMs attached to a TI-99/4 home computer

#ifndef TI994ROM_H
#define TI994ROM_H

#include "..\EmulatorSupport\peripheral.h"
#include "Classic99ROM.h"

// CPU ROM memory
class TI994ROM : public Classic99ROM {
public:
    TI994ROM(Classic99System *core);
    virtual ~TI994ROM();
};


#endif
