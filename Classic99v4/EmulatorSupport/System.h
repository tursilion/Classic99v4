// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef EMULATOR_SUPPORT_SYSTEM_H
#define EMULATOR_SUPPORT_SYSTEM_H

#include "peripheral.h"
#include "debuglog.h"

// TODO: probably need a smart pointered list of possible systems,
// the active system (getCore) needs to be a smart pointer (shared_ptr<T>),
// and a smart pointered list of loaded peripheral devices

// a dummy peripheral to respond where nothing is mapped
extern Classic99Peripheral dummyPeripheral;

// maps an address or IO to a peripheral class
class PeripheralMap {
public:
    PeripheralMap() { who = &dummyPeripheral; addr = 0; }
    PeripheralMap(Classic99Peripheral *inWho, int inAddr) { who = inWho; addr = inAddr; }

    void updateMap(Classic99Peripheral *inWho, int inAddr) { who = inWho; addr = inAddr; }

protected:
    Classic99Peripheral *who;   // which peripheral to call
    int addr;                   // what address to tell the peripheral

    friend class Classic99System;
};

// this defines the pure virtual base class for a Classic99System - all systems
// must derive from this class. This is used largely to provide an
// interface for creating and registering the system components.
class Classic99System {
public:
    Classic99System();
    virtual ~Classic99System();

    // Setup the system, create everything needed
    virtual bool initSystem() = 0;      // create the needed components and register them
    // tear down the system, destroy everything created. Before being called,
    // everyone else needs to have unmapOutputs called so they forget about us.
    virtual bool deInitSystem() = 0;    // undo initSystem, as needed
    virtual bool runSystem(int microSeconds) = 0;   // run the system for a fixed amount of time

    // allocating memory space to devices
    bool claimRead(int sysAdr, Classic99Peripheral *periph, int periphAdr);
    bool claimWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr);
    bool claimIORead(int sysAdr, Classic99Peripheral *periph, int periphAdr);
    bool claimIOWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr);
                                                    
    // The CPUs normally default into these maps, but some CPUs might have their own handlers
    int readMemoryByte(int address, bool allowSideEffects);
    void writeMemoryByte(int address, bool allowSideEffects, int data);
    int readIOByte(int address, bool allowSideEffects);
    void writeIOByte(int address, bool allowSideEffects, int data);

    // debug interface - mainly for breakpoints, part of the main loop
    void processDebug();

    // locks/unlocks
    void lock() {
        // lock ourselves
        al_lock_mutex(coreLock);
    }
    void unlock() {
        // unlock ourselves
        al_unlock_mutex(coreLock);
    }

private:
    ALLEGRO_MUTEX *coreLock;      // our object lock

    // derived classes are expected to allocate these
    // read and write are separate because some systems are clever...
    PeripheralMap *memorySpaceRead;
    PeripheralMap *memorySpaceWrite;
    PeripheralMap *ioSpaceRead;
    PeripheralMap *ioSpaceWrite;
    int memorySize;     // power of 2 mask
    int ioSize;         // power of 2 mask
};

// make theCore pointer available - make sure to check for NULL. If not null, theCore is locked, so unlock when done
// You MUST "unlock" for every lockCore
Classic99System *lockCore() { if (theCore == NULL) return NULL; theCore->lock(); return theCore; }

#endif
