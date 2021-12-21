// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4((A)) home computer

#include "TI994AGROM.h"

extern const uint8_t TI994AGROMData[8192*3];

#define INCLUDE_DEMONSTRATION
#ifdef INCLUDE_DEMONSTRATION
extern const uint8_t DEMOG[32768];
#endif

// not much needed for construction, except pointing to the data
TI994AGROM::TI994AGROM(Classic99System *core) 
    : Classic99GROM(core, TI994AGROMData, 8192*3, 0)
{
#ifdef INCLUDE_DEMONSTRATION
    LoadAdditionalData(DEMOG, 32768, 0x6000);
#endif
}

TI994AGROM::~TI994AGROM() {
}
