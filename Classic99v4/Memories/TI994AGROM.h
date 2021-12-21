// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4((A)) home computer

#ifndef TI994AGROM_H
#define TI994AGROM_H

#include "Classic99GROM.h"

class TI994AGROM : public Classic99GROM {
public:
    TI994AGROM(Classic99System *core);
    virtual ~TI994AGROM();
};


#endif
