// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef EMULATOR_SUPPORT_SYSTEM_H
#define EMULATOR_SUPPORT_SYSTEM_H

#include <atomic>
#include "tv.h"
#include "speaker.h"
#include "debuglog.h"

// TODO: probably need a smart pointered list of possible systems,
// the active system (getCore) needs to be a smart pointer (shared_ptr<T>),
// and a smart pointered list of loaded peripheral devices

// absolute maximums so I can have a set of arrays
#define MAX_CPUS 16

// a dummy peripheral to respond where nothing is mapped
class Classic99Peripheral;
extern Classic99Peripheral dummyPeripheral;

// track type of memory access
enum MEMACCESSTYPE {
    ACCESS_READ = 0,    // normal read (or write)
    ACCESS_WRITE = 0,   // normal write (or read)
    ACCESS_RMW,         // read-before-write access, do not breakpoint, but count cycles
    ACCESS_FREE         // internal access, do not count or breakpoint
};

// TODO: system needs a save state too - in fact it probably triggers all the other save states
// But remember to save off the interrupt state and stuff that System actually owns!

// maps an address or IO to a peripheral class
class PeripheralMap {
public:
    PeripheralMap() { who = &dummyPeripheral; addr = 0; waitStates = 0; }
    PeripheralMap(Classic99Peripheral *inWho, int inAddr, int inWait) { who = inWho; addr = inAddr; waitStates = inWait; }

    // pass nullptr or -1 to leave unchanged
    void updateMap(Classic99Peripheral *inWho, int inAddr, int inWait) { 
        if (nullptr != inWho) who = inWho; 
        if (-1 != inAddr) addr = inAddr; 
        if (-1 != inWait) waitStates = inWait; 
    }

protected:
    Classic99Peripheral *who;   // which peripheral to call
    int addr;                   // what address to tell the peripheral
    int waitStates;             // how many cycles does accessing this peripheral cost in wait states?
                                // note the peripheral itself may add additional cycles, this is system-wide.

    friend class Classic99System;
};

// this defines the pure virtual base class for a Classic99System - all systems
// must derive from this class. This is used largely to provide an
// interface for creating and registering the system components.
class Classic99System {
public:
    Classic99System();
    virtual ~Classic99System();

    // disallow copy and assign constructors
    Classic99System(Classic99System const& other) = delete;
    Classic99System &operator=(Classic99System const &other) = delete;

    // Setup the system, create everything needed
    virtual bool initSystem() = 0;      // create the needed components and register them
    // tear down the system, destroy everything created. Before being called,
    // everyone else needs to have unmapOutputs called so they forget about us.
    virtual bool deInitSystem() = 0;    // undo initSystem, as needed
    virtual bool runSystem(int microSeconds) = 0;   // run the system for a fixed amount of time

    // allocating memory space to devices
    // TODO: this array system won't scale to 32-bit systems...??
    // I guess we could allocate blocks instead of bytes in those cases?
    bool claimRead(int sysAdr, Classic99Peripheral *periph, int periphAdr);
    bool claimWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr);
    bool claimIORead(int sysAdr, Classic99Peripheral *periph, int periphAdr);
    bool claimIOWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr);
                                                    
    // The CPUs normally default into these maps, but some CPUs might have their own handlers
    uint8_t readMemoryByte(int address, volatile long &cycles, MEMACCESSTYPE rmw);
    void writeMemoryByte(int address, volatile long &cycles, MEMACCESSTYPE rmw, int data);
    uint8_t readIOByte(int address, volatile long &cycles, MEMACCESSTYPE rmw);
    void writeIOByte(int address, volatile long &cycles, MEMACCESSTYPE rmw, int data);

    // access to the I/O
    Classic99TV *getTV() { return theTV; }
    Classic99Speaker *getSpeaker() { return theSpeaker; }

    // debug interface - mainly for breakpoints, part of the main loop
    void processDebug();
    void triggerBreakpoint();

    // hold/halt/freeze line, we need to track who - should be passed in as
    // peripheral number. That assumes no more than 32 peripherals, is that ok?
    // Hold stays active till all peripherals release.
    // Note the requesting device doesn't know which CPU its attached to,
    // so dealing with the holds in multi-CPU cases may need different work,
    // but that is definitely system-specific. So these default cases assume
    // one hold bus, one interrupt bus, one NMI line.
    virtual void requestHold(int device) {
        if (device < sizeof(int)) {
            hold |= 1<<device;
        } else {
            debug_write("Invalid peripheral index for hold %d", device);
        }
    }
    virtual bool getHoldStatus(int device) {
        if (-1 == device) {
            return (0 != hold);
        } else {
            return ((hold&(1<<device)) ? true:false );
        }
    }
    virtual bool releaseHold(int device) {
        if (device < sizeof(int)) {
            hold &= ~( 1<<device );
        } else {
            debug_write("Invalid peripheral index for hold release %d", device);
        }
        return true;
    }

    // interrupts - we don't need to track who, but we do need level and which CPU
    // CPU will clear this when serviced - is that okay? The 9900 is level based, so
    // for now, we will assume that's what we want to do here.
    virtual void requestInt(int level) {
        intReqLevel |= (1<<level);
    }
    // I don't think we care who requested this either...?
    virtual void requestNMI() { nmiReq = true; }
    // covers both kinds
    virtual bool interruptPending() { return (intReqLevel != 0) || (nmiReq); }
    // functions for the CPUs
    virtual bool getAndClearIntReq(int level) {
        int32_t n = 1<<level;  
        if (intReqLevel.fetch_and(~n) & n) {
            return true;
        } else {
            return false;
        }
    }
    virtual bool getAndClearNMI() { return nmiReq.exchange(false); }

protected:
    ALLEGRO_MUTEX *coreLock;      // our object lock

    // derived classes are expected to allocate these
    // read and write are separate because some systems are clever...
    PeripheralMap *memorySpaceRead;
    PeripheralMap *memorySpaceWrite;
    PeripheralMap *ioSpaceRead;
    PeripheralMap *ioSpaceWrite;
    int memorySize;     // power of 2 mask
    int ioSize;         // power of 2 mask

    double currentTimestamp;
    Classic99TV *theTV;
    Classic99Speaker *theSpeaker;

private:
    // we have a single set of I/O devices that the peripherals can talk to
    // These MUST be initialized in the derived class constructors, and MUST point to something,
    // even a dummy if needed.
    // sound
    // inputs
    // etc...

    // true means active, no matter the hardware level
    std::atomic<uint32_t> hold;         // Per-device bitmask - Hold, halt, freeze, etc - a line that requests the CPU or similar to stop
    std::atomic<uint32_t> holdack;      // the inverse - an output signal that the CPU is honoring the hold

    // use atomic_or() to update
    std::atomic<uint32_t> intReqLevel;  // a bitmask of active IRQ levels (1=level 1, 2=level 2, 4=level 3, etc)
    std::atomic<bool> nmiReq;      // a non-maskable interrupt request to the CPU

};

#endif
