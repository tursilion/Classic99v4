// Classic99 v4xx - Copyright 2020 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef EMULATOR_SUPPORT_PERIPHERAL_H
#define EMULATOR_SUPPORT_PERIPHERAL_H

#include <allegro5/allegro.h>

// The base peripheral interface allows for instantiating and destroying
// a peripheral instance, as well as providing a debug interface for it

// peripheral IO types (bitmask)
// TODO: This lets the system see what can be hooked up, when
// I have that ability, anyway.
// Intended for things like serial ports, modems, modem emulators,
// printers, and maybe even direct connections between devices...
#define PERIPHERAL_IO_NONE           0x00
#define PERIPHERAL_IO_SERIAL1_IN     0x01
#define PERIPHERAL_IO_SERIAL1_OUT    0x02
#define PERIPHERAL_IO_PARALLEL1_IN   0x04
#define PERIPHERAL_IO_PARALLEL1_OUT  0x08
#define PERIPHERAL_IO_SERIAL2_IN     0x10
#define PERIPHERAL_IO_SERIAL2_OUT    0x20
#define PERIPHERAL_IO_PARALLEL2_IN   0x40
#define PERIPHERAL_IO_PARALLEL2_OUT  0x80

class Classic99Peripheral {
public:
    Classic99Peripheral() {
        // create the lock object - we can't be certain that
        // we don't need a recursive mutex since others can use it
        periphLock = al_create_mutex_recursive();
    }
    virtual ~Classic99Peripheral() {
        // release the lock object
        al_destroy_mutex(periphLock);
    };

    // TODO: timing interface
    // TODO: debug interface

protected:
    virtual void lock() {
        // lock ourselves
        al_lock_mutex(periphLock);
    }

    virtual void unlock() {
        // unlock ourselves
        al_unlock_mutex(periphLock);
    }

private:
    ALLEGRO_MUTEX *periphLock;      // our object lock

    friend class Classic99System;   // allowed access to our lock methods
}


#endif
