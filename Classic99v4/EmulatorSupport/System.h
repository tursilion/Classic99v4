// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef EMULATOR_SUPPORT_SYSTEM_H
#define EMULATOR_SUPPORT_SYSTEM_H

#include "peripheral.h"
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
    uint8_t readMemoryByte(int address, int &cycles, MEMACCESSTYPE rmw);
    void writeMemoryByte(int address, int &cycles, MEMACCESSTYPE rmw, int data);
    uint8_t readIOByte(int address, int &cycles, MEMACCESSTYPE rmw);
    void writeIOByte(int address, int &cycles, MEMACCESSTYPE rmw, int data);

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
    }

    // interrupts - we don't need to track who, but we do need level and which CPU
    // CPU will clear this when serviced - is that okay? Some CPUs are level based
    // and some CPUs are edge based, this is going to treat everything as edge based...
    // TODO: we probably want level based someday, which means edge trigger moves
    // into the CPU, and we need to add methods to release the request...
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

private:
    // derived classes are expected to allocate these
    // read and write are separate because some systems are clever...
    PeripheralMap *memorySpaceRead;
    PeripheralMap *memorySpaceWrite;
    PeripheralMap *ioSpaceRead;
    PeripheralMap *ioSpaceWrite;
    int memorySize;     // power of 2 mask
    int ioSize;         // power of 2 mask

    // true means active, no matter the hardware level
    std::atomic<uint32_t> hold;         // Per-device bitmask - Hold, halt, freeze, etc - a line that requests the CPU or similar to stop
    std::atomic<uint32_t> holdack;      // the inverse - an output signal that the CPU is honoring the hold

    // use atomic_or() to update
    std::atomic<uint32_t> intReqLevel;  // a bitmask of active IRQ levels (1=level 1, 2=level 2, 4=level 3, etc)
    std::atomic<bool> nmiReq;      // a non-maskable interrupt request to the CPU

};

#endif
