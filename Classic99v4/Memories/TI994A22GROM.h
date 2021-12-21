// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4A v2.2 home computer

#ifndef TI994A22GROM_H
#define TI994A22GROM_H

#include "Classic99GROM.h"

class TI994A22GROM : public Classic99GROM {
public:
    TI994A22GROM(Classic99System *core);
    virtual ~TI994A22GROM();
};


#endif
