// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include <cstdio>
#include "peripheral.h"
#include "automutex.h"
#include "debuglog.h"

// singleton dummy object for addresses with nothing mapped
Classic99Peripheral dummyPeripheral(NULL);
// singleton dummy breakpoint object with nothing set
BREAKPOINT dummyBreakpoint;

#ifdef ALLEGRO_WINDOWS
#define snprintf _snprintf
#endif

// called from the main code to request a breakpoint be added
void Classic99Peripheral::addBreakpoint(BREAKPOINT &inBreak) {
    autoMutex lock(periphLock);

    // find an empty slot for the breakpoint
    for (int idx=0; idx<MAX_BREAKPOINTS; ++idx) {
        if (breaks[idx].typeMask == 0) {
            // here we go...
            breaks[idx] = inBreak;
            break;
        }
    }
}

// called from the main code to request a breakpoint be removed
void Classic99Peripheral::removeBreakpoint(BREAKPOINT &inBreak) {
    autoMutex lock(periphLock);

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
}

// called from the peripheral memory access to check whether breakpoints are hit
// we expect to already be locked
void Classic99Peripheral::testBreakpoint(bool isRead, int addr, bool isIO, int data) {
    // assumes a non-sparse breaks[] array!
    for (int idx=0; idx<MAX_BREAKPOINTS; ++idx) {
        if (breaks[idx].typeMask == 0) break;
            if (breaks[idx].typeMask & BREAKPOINT::BREAKPOINT_MASK_PERIPHERAL) continue;            // skip non-CPU address breaks
            if (!breaks[idx].CheckRange(addr, 0)) continue;                                         // check address match (most likely false)
            if ((isRead)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_READ)==0)) continue;   // check for read
            if ((!isRead)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_WRITE)==0)) continue; // check for write
            if ((isIO)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_IO)==0)) continue;       // check for IO
            if ((!isIO)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_IO)!=0)) continue;      // check for not IO
            // we've narrowed it down a lot at this point, check the less likely things...
            if (breaks[idx].page != page) continue;

            // TODO: add ignore console breakpoints (meaning ALL breakpoints need to check the primary CPU address,
            // which is in InterestingData, and we need a range check on the system core to determine ROM addresses...
            // maybe this can also be in InterestingData, along with the ignore rom hits flag.)

            // So we have confirmed whether it's read or write, IO or not, all that is left
            // is to compare the data (and a mere access breakpoint will have a zero mask)
            if ((breaks[idx].dataMask != 0) && ((data&breaks[idx].dataMask) != (breaks[idx].data&breaks[idx].dataMask))) continue;

            theCore->triggerBreakpoint();
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
void Classic99Peripheral::setIndex(const char *name, int in) {
    autoMutex lock(periphLock);

    if (NULL != name) {
        snprintf(formattedName, sizeof(formattedName), "%s_%d", name, in);
    } else {
        char *p = strchr(formattedName, '_');
        if (NULL == p) {
            debug_write("Name format invalid");
        } else {
            *(++p) = '\0';
            snprintf(p, sizeof(formattedName)-(p-formattedName), "%d", in);
        }
    }
}

// TODO: the function that calls the save/restore state function should
// verify the assertions, and refuse to save/load state if they are untrue:
// double = 8 bytes
// int = 4 bytes
// short = 2 bytes
// unsigned int = 4 bytes
// unsigned short = 2 bytes
// bool is handled manually, so that one is okay

// save state helpers
void Classic99Peripheral::saveStateVal(unsigned char *&buffer, bool n) {
    *(buffer++) = n?1:0;
}
void Classic99Peripheral::saveStateVal(unsigned char *&buffer, double n) {
    memcpy(buffer, &n, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::saveStateVal(unsigned char *&buffer, int n) {
    memcpy(buffer, &n, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::saveStateVal(unsigned char *&buffer, short n) {
    memcpy(buffer, &n, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::saveStateVal(unsigned char *&buffer, unsigned int n) {
    memcpy(buffer, &n, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::saveStateVal(unsigned char *&buffer, unsigned short n) {
    memcpy(buffer, &n, sizeof(n));
    buffer+=sizeof(n);
}

void Classic99Peripheral::loadStateVal(unsigned char *&buffer, bool &n) {
   n = *(buffer++) ? true : false;
}
void Classic99Peripheral::loadStateVal(unsigned char *&buffer, double &n) {
    memcpy(&n, buffer, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::loadStateVal(unsigned char *&buffer, int &n) {
    memcpy(&n, buffer, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::loadStateVal(unsigned char *&buffer, short &n) {
    memcpy(&n, buffer, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::loadStateVal(unsigned char *&buffer, unsigned int &n) {
    memcpy(&n, buffer, sizeof(n));
    buffer+=sizeof(n);
}
void Classic99Peripheral::loadStateVal(unsigned char *&buffer, unsigned short &n) {
    memcpy(&n, buffer, sizeof(n));
    buffer+=sizeof(n);
}
