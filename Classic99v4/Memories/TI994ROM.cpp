// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the ROMs attached to a TI-99/4 home computer

#include "TI994ROM.h"

extern const uint8_t TI994ROMData[8192];

// not much needed for construction, except pointing to the data
TI994ROM::TI994ROM(Classic99System *core) 
    : Classic99ROM(core, TI994ROMData, 8192)
{
}
TI994ROM::~TI994ROM() {
}

