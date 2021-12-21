// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4((A)) home computer

#include "TI994AGROM.h"

extern const uint8_t TI994AGROMData[8192*3];

// not much needed for construction, except pointing to the data
TI994AGROM::TI994AGROM(Classic99System *core) 
    : Classic99GROM(core, TI994AGROMData, 8192*3, 0)
{
}

TI994AGROM::~TI994AGROM() {
}
