// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4 home computer

#include "TI994GROM.h"

extern const uint8_t TI994GROMData[8192*3];

#define INCLUDE_DEMONSTRATION
#ifdef INCLUDE_DEMONSTRATION
extern const uint8_t DEMOG[32768];
#endif

// not much needed for construction, except pointing to the data
TI994GROM::TI994GROM(Classic99System *core) 
    : Classic99GROM(core, TI994GROMData, 8192*3, 0)
{
#ifdef INCLUDE_DEMONSTRATION
    LoadAdditionalData(DEMOG, 32768, 0x6000);
#endif
}

TI994GROM::~TI994GROM() {
}
