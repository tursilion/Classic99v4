// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class subclasses TI994 to add the better keyboard
// and VDP and ROMs, otherwise it's the same.

#include "TI994A.h"
#include "../../EmulatorSupport/interestingData.h"
#include "../../Memories/TI994AGROM.h"
#include "../../Memories/TI994AROM.h"
#include "../../VDPs/TMS9918A.h"
#include "../../Keyboard/kb_994A.h"

TI994A::TI994A()
    : TI994()
{
    // set shared interesting data
    setInterestingData(INDIRECT_KEY_INJECT_ADDRESS, 0x478);
    setInterestingData(INDIRECT_KEY_KEY_ADDRESS, 0x83e0);
    setInterestingData(INDIRECT_KEY_STATUS_ADDRESS, 0x83ec);
}

TI994A::~TI994A() {
}

// this virtual function is used to subclass the various types of TI99 - anything that varies is
// loaded and initialized in here.
bool TI994A::initSpecificSystem() {
    pGrom = new TI994AGROM(this);
    if (!pGrom->init(0)) return false;
    pRom = new TI994AROM(this);
    if (!pRom->init(0)) return false;
    pVDP = new TMS9918A(this);
    if (!pVDP->init(0)) return false;
    pKey = new KB994A(this);
    if (!pKey->init(0)) return false;

    // map in the alpha lock key output on bit 21
    for (int idx=0; idx<0x800; idx+=20) {
        claimIOWrite(idx+21, pKey, 21);
    }

    return true;
}
