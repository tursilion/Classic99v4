// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class subclasses TI994 to add the better keyboard
// and VDP and ROMs, otherwise it's the same.

#include "TI994A.h"
#include "..\..\EmulatorSupport\interestingData.h"
#include "..\..\Memories\TI994AGROM.h"
#include "..\..\Memories\TI994AROM.h"
#include "..\..\VDPs\TMS9918A.h"
#include "..\..\Keyboard\kb_994A.h"

TI994A::TI994A()
    : TI994()
{
}

TI994A::~TI994A() {
}

// this virtual function is used to subclass the various types of TI99 - anything that varies is
// loaded and initialized in here.
bool TI994A::initSpecificSystem() {
    pGrom = new TI994AGROM(this);
    pGrom->init(0);
    pRom = new TI994AROM(this);
    pRom->init(0);
    pVDP = new TMS9918A(this);
    pVDP->init(0);
    pKey = new KB994A(this);
    pKey->init(0);

    // map in the alpha lock key output on bit 21
    const int KeyWait = 0;
    for (int idx=0; idx<0x800; idx+=20) {
        claimIOWrite(idx+21, pKey, 21, KeyWait);
    }

    return true;
}
