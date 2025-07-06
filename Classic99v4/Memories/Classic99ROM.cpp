// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates ROMs attached to a CPU
// it is intended to be overridden and the correct ROMs loaded in the constructor, loading pSrcData and dataSize

#include "Classic99ROM.h"

// not much needed for construction, except pointing to the data
Classic99ROM::Classic99ROM(Classic99System *core, const uint8_t *pInDataSrc, int inDataSize) 
    : Classic99Peripheral(core)
    , pDataSrc(pInDataSrc)
    , dataSize(inDataSize)
    , pData(nullptr)
{
}
Classic99ROM::~Classic99ROM() {
}

uint8_t Classic99ROM::read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) {
    // there are no extra cycles used by the TI's 16-bit ROM - this is why we don't need the write function
    (void)isIO;
    (void)((long)cycles);    // still unused, dumb gcc workaround
    (void)rmw;

    // in theory, the address should be restricted to what we asked for, but in debug mode we better check
#ifdef _DEBUG
    if (addr >= dataSize) {
        // this is a mapping error in the system initialization, most likely
        debug_write("ERROR: MEMORY ACCESS OUT OF RANGE!");
        return 0;
    }
#endif

    return pData[addr];
}

bool Classic99ROM::init(int idx) {
    setIndex("ROM", idx);
    // get the user data into our own memory space
    if (dataSize > 0) {
        if (nullptr != pData) {
            free(pData);
        }
        pData = (uint8_t*)malloc(dataSize);
        memcpy(pData, pDataSrc, dataSize);
    } else {
        debug_write("Failed to initialize ROM - no data provided.");
    }

    return true;
}

bool Classic99ROM::operate(double timestamp) {
    // nothing special to do here
    return true;
}

bool Classic99ROM::cleanup() {
    // nothing special to do here
    return true;
}

void Classic99ROM::getDebugSize(int &x, int &y, int user) {
    // not sure at the moment how this is going to work... might need to add
    // a view method as well as a debug window... or at least a variable offset
    x=0;
    y=0;
}

void Classic99ROM::getDebugWindow(char *buffer, int user) {
    // nothing at the moment
}

int Classic99ROM::saveStateSize() {
    return dataSize;
}

bool Classic99ROM::saveState(unsigned char *buffer) {
    memcpy(buffer, pData, dataSize);
    return true;
}

bool Classic99ROM::restoreState(unsigned char *buffer) {
    if (nullptr != pData) {
        free(pData);
    }
    pData = (uint8_t*)malloc(dataSize);
    memcpy(pData, buffer, dataSize);
    return true;
}

