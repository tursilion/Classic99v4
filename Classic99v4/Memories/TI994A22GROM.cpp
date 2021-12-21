// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4((A)) home computer

#include "TI994A22GROM.h"

extern const uint8_t TI994A22GROMData[8192*3];

// not much needed for construction, except pointing to the data
TI994A22GROM::TI994A22GROM(Classic99System *core) 
    : Classic99GROM(core, TI994A22GROMData, 8192*3, 0)
{
}

TI994A22GROM::~TI994A22GROM() {
}
