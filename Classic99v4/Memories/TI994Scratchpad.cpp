// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the 256 byte scratchpad RAM attached to a TI-99/4 home computer

#include "TI994Scratchpad.h"

// not much needed for construction
TI994Scratchpad::TI994Scratchpad(Classic99System *core) 
    : Classic99Peripheral(core)
{
    // not necessarily zeroed on power up in real hardware
    memset(Data, 0, sizeof(Data));
}
TI994Scratchpad::~TI994Scratchpad() {
}

uint8_t TI994Scratchpad::read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) {
    // there are no extra cycles used by the TI's 16-bit RAM
    (void)isIO;
    (void)cycles;
    (void)rmw;

    // in theory, the address should be restricted to what we asked for, but in debug mode we better check
#ifdef _DEBUG
    if (addr > 0xFF) {
        // we only have 256 bytes of memory!
        debug_write("ERROR: SCRATCH MEMORY ACCESS OUT OF RANGE!");
        return 0;
    }
#endif

    return Data[addr];
}

void TI994Scratchpad::write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) {
    // there are no extra cycles used by the TI's 16-bit RAM
    (void)isIO;
    (void)cycles;
    (void)rmw;

    // in theory, the address should be restricted to what we asked for, but in debug mode we better check
#ifdef _DEBUG
    if (addr > 0xFF) {
        // we only have 256 bytes of memory!
        debug_write("ERROR: SCRATCH MEMORY ACCESS OUT OF RANGE!");
        return;
    }
#endif

    Data[addr] = data;
}

bool TI994Scratchpad::init(int idx) {
    // nothing special to do here
    return true;
}

bool TI994Scratchpad::operate(double timestamp) {
    // nothing special to do here
    return true;
}

bool TI994Scratchpad::cleanup() {
    // nothing special to do here
    return true;
}

void TI994Scratchpad::getDebugSize(int &x, int &y) {
    // not sure at the moment how this is going to work... might need to add
    // a view method as well as a debug window... or at least a variable offset
    x=0;
    y=0;
}

void TI994Scratchpad::getDebugWindow(char *buffer) {
    // nothing at the moment
}

int TI994Scratchpad::saveStateSize() {
    return MEMSIZE;
}

bool TI994Scratchpad::saveState(unsigned char *buffer) {
    memcpy(buffer, Data, MEMSIZE);
    return true;
}

bool TI994Scratchpad::restoreState(unsigned char *buffer) {
    memcpy(Data, buffer, MEMSIZE);
    return true;
}
