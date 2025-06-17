// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// Implementation of the TMS9918 VDP for Classic99

// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef TMS9918A_H
#define TMS9918A_H

#ifndef CONSOLE_BUILD
#include <raylib.h>
#endif

#include <atomic>
#include "../EmulatorSupport/System.h"
#include "../EmulatorSupport/peripheral.h"
#include "TMS9918.h"

// TODO: At the moment I haven't done any of the work needed to split the two classes up,
// so this is just running the whole thing. ;)

class TMS9918A : public TMS9918 {
public:
    TMS9918A() = delete;
    TMS9918A(Classic99System *core);
    virtual ~TMS9918A() ;
};
#endif

