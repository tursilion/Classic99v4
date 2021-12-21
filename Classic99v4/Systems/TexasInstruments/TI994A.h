// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef TI994A_H
#define TI994A_H

// This class builds a TI-99/4((A)) (1981) machine
// by subclassing the TI-99/4 class

#include "TI994.h"

// define the actual system class
class TI994A : public TI994 {
public:
    TI994A();
    virtual ~TI994A();

    // Setup the system, create everything needed
    //bool initSystem();      // create the needed components and register them
    // tear down the system, destroy everything created. Before being called,
    // everyone else needs to have unmapOutputs called so they forget about us.
    //bool deInitSystem();    // undo initSystem, as needed
    //bool runSystem(int microSeconds);   // run the system for a fixed amount of time

    bool initSpecificSystem() override;  // load the parts of a TI/99 that can vary between models
};

#endif
