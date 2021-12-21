// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4((A)) home computer

#ifndef TI994AROM_H
#define TI994AROM_H

#include "Classic99ROM.h"

// CPU ROM memory
class TI994AROM : public Classic99ROM {
public:
    TI994AROM(Classic99System *core);
    virtual ~TI994AROM();
};


#endif
