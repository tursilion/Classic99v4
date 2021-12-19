// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class builds a TI-99/4 (1979) machine
// Later versions of the TI-99/4 subclass this one where needed
// as they are all very, very similar - only the keyboard, VDP
// and ROMs differ.

#include "..\..\EmulatorSupport\System.h"
#include "..\..\CPUs\tms9900.h"
#include "..\..\Memories\TI994GROM.h"
#include "..\..\Memories\TI994ROM.h"
#include "..\..\Memories\TI994Scratchpad.h"
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

private:
    // the hardware we need to create
    TMS9900 *pCPU;
    TI994GROM *pGrom;
    TI994ROM *pRom;
    TI994Scratchpad *pScratch;
    TMS9918 *pVDP;
};

