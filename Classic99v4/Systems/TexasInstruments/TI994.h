// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef TI994_H
#define TI994_H

// This class builds a TI-99/4 (1979) machine
// Later versions of the TI-99/4 subclass this one where needed
// as they are all very, very similar - only the keyboard, VDP
// and ROMs differ.

#include "..\..\EmulatorSupport\System.h"
#include "..\..\CPUs\tms9900.h"
#include "..\..\Memories\TI994Scratchpad.h"
#include "..\..\Memories\Classic99GROM.h"
#include "..\..\Memories\Classic99ROM.h"
#include "..\..\Keyboard\TIKeyboard.h"
#include "..\..\VDPs\TMS9918.h"

// define the actual system class
class TI994 : public Classic99System {
public:
    TI994();
    virtual ~TI994();

    // Setup the system, create everything needed
    bool initSystem();      // create the needed components and register them
    // tear down the system, destroy everything created. Before being called,
    // everyone else needs to have unmapOutputs called so they forget about us.
    bool deInitSystem();    // undo initSystem, as needed
    bool runSystem(int microSeconds);   // run the system for a fixed amount of time

    virtual bool initSpecificSystem();  // load the parts of a TI/99 that can vary between models

protected:
    // the hardware we need to create
    TMS9900 *pCPU;
    Classic99GROM *pGrom;
    Classic99ROM *pRom;
    TI994Scratchpad *pScratch;
    TMS9918 *pVDP;                      // will be a 9918A on the 99/4A variant, could also be an F18A eventually
    TIKeyboard *pKey;
};

#endif
