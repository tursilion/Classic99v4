// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the ROMs attached to a TI-99/4((A)) home computer

#include "TI994AROM.h"

extern const uint8_t TI994AROMData[8192];

// not much needed for construction, except pointing to the data
TI994AROM::TI994AROM(Classic99System *core) 
    : Classic99ROM(core, TI994AROMData, 8192)
{
}
TI994AROM::~TI994AROM() {
}

