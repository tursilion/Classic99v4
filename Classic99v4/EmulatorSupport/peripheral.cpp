// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include "peripheral.h"
#include <stdio.h>

// singleton dummy object for addresses with nothing mapped
Classic99Peripheral dummyPeripheral;
// singletone dummy breakpoint object with nothing set
BREAKPOINT dummyBreakpoint;

// called from the main code to request a breakpoint be added
void Classic99Peripheral::addBreakpoint(BREAKPOINT &inBreak) {
    lock();
    // find an empty slot for the breakpoint
    for (int idx=0; idx<MAX_BREAKPOINTS; ++idx) {
        if (breaks[idx].typeMask == 0) {
            // here we go...
            breaks[idx] = inBreak;
            break;
        }
    }
    unlock();
}

// called from the main code to request a breakpoint be removed
void Classic99Peripheral::removeBreakpoint(BREAKPOINT &inBreak) {
    lock();
    // look for a match and remove it - must keep array non-sparse
    for (int idx=0; idx<MAX_BREAKPOINTS; ++idx) {
        if (breaks[idx] == inBreak) {
            // move all the subsequent ones down
            for (int i2=idx+1; i2<MAX_BREAKPOINTS; ++i2) {
                breaks[i2-1] = breaks[i2];
            }
            // and clear the last one
            breaks[MAX_BREAKPOINTS-1].typeMask = 0;
            break;
        }
    }
    unlock();
}

// called from the peripheral memory access to check whether breakpoints are hit
// we expect to already be locked
void Classic99Peripheral::testBreakpoint(bool isRead, int addr, bool isIO, int data) {
    // assumes a non-sparse breaks[] array!
    for (int idx=0; idx<MAX_BREAKPOINTS; ++idx) {
        if (breaks[idx].typeMask == 0) break;
            if (addr != breaks[idx].address) continue;                                              // check address match (most likely false)
            if ((isRead)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_READ)==0)) continue;   // check for read
            if ((!isRead)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_WRITE)==0)) continue; // check for write
            if ((isIO)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_IO)==0)) continue;       // check for IO
            if ((!isIO)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_IO)!=0)) continue;      // check for not IO
            // we've narrowed it down a lot at this point, check the less likely things...
            if (breaks[idx].page != page) continue;
            if ((data&breaks[idx].dataMask) != (breaks[idx].data&breaks[idx].dataMask)) continue;

            // TODO: ask the core to trigger a breakpoint
            //getCore()->triggerBreakpoint();
            break;
    }
}

// return a copy of the requested breakpoint index - this is just for display so not locking for now... we're called a lot
// if we need to lock, then lock externally. But in theory the same guy who adds/removes should be calling this to list
BREAKPOINT Classic99Peripheral::getBreakpoint(int idx) {
    if (idx>MAX_BREAKPOINTS) return dummyBreakpoint;
    return breaks[idx];
}

// setting the index generates the output name
void Classic99Peripheral::setIndex(int in) {
    lock();

    index = in;
    _snprintf(formattedName, sizeof(formattedName), "%s_%d", myName, index);

    unlock();
}

