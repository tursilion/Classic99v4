// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the ROMs attached to a TI-99/4 home computer

#include "TI994ROM.h"

extern const uint8_t TI994ROMData[8192];

// not much needed for construction, except pointing to the data
TI994ROM::TI994ROM(Classic99System *core) 
    : Classic99Peripheral(core)
{
    pData = TI994ROMData;
}
TI994ROM::~TI994ROM() {
}

uint8_t TI994ROM::read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) {
    // there are no extra cycles used by the TI's 16-bit ROM - this is why we don't need the write function
    (void)isIO;
    (void)cycles;
    (void)rmw;

    // in theory, the address should be restricted to what we asked for, but in debug mode we better check
#ifdef _DEBUG
    if (addr > 0x1FFF) {
        // we only have 8k of memory!
        debug_write("ERROR: MEMORY ACCESS OUT OF RANGE!");
        return 0;
    }
#endif

    return pData[addr];
}

bool TI994ROM::init(int idx) {
    // nothing special to do here
    return true;
}

bool TI994ROM::operate(double timestamp) {
    // nothing special to do here
    return true;
}

bool TI994ROM::cleanup() {
    // nothing special to do here
    return true;
}

void TI994ROM::getDebugSize(int &x, int &y) {
    // not sure at the moment how this is going to work... might need to add
    // a view method as well as a debug window... or at least a variable offset
    x=0;
    y=0;
}

void TI994ROM::getDebugWindow(char *buffer) {
    // nothing at the moment
}

int TI994ROM::saveStateSize() {
    // no save state needed
    // TODO: I wonder if that's true - how do we get ROMs back on save state?
    // Also, accessing const arrays means we can't change them. Do we need CPU memory after all?
    return 0;
}

bool TI994ROM::saveState(unsigned char *buffer) {
    // should not be called...
    return true;
}

bool TI994ROM::restoreState(unsigned char *buffer) {
    // nothing to do
    return true;
}

