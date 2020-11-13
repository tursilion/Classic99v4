// Classic99 v4xx - Copyright 2020 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef EMULATOR_SUPPORT_SYSTEM_H
#define EMULATOR_SUPPORT_SYSTEM_H

#include "peripheral.h"
#include "cpu.h"
#include "vdp.h"
#include "psg.h"

// maximum number of CPUs supported
// Note this includes add-ons and peripherals that perform their own processing
#define MAX_CPUS 5

// maximum number of PSGs supported
#define MAX_PSGS 2

// this defines the base class for a Classic99System - all systems
// must derive from this class. This is used largely to provide an
// interface for creating and registering the system components.
class Classic99System {
public:
    Classic99System() {
        mainVDP = nullptr;
        for (int i=0; i<MAX_CPUS; ++i) {
            mainCPU[i] = nullptr;
        }
        for (int i=0; i<MAX_PSGS; ++i) {
            mainPSG[i] = nullptr;
        }
    };
    virtual ~Classic99System() {
        // before destroying, make sure everyone has forgotten about us!
        if (mainVDP != nullptr) {
            delete mainVDP;
        }
        for (int i=0; i<MAX_CPUS; ++i) {
            if (mainCPU[i] != nullptr) {
                delete mainCPU[i];
            }
        }
        for (int i=0; i<MAX_PSGS; ++i) {
            if (mainPSG[i] != nullptr) {
                delete mainPSG[i];
            }
        }
    }

    // Setup the system, create everything needed
    virtual bool initSystem() = 0;      // create the needed components and register them
    // tear down the system, destroy everything created. Before being called,
    // everyone else needs to have unmapOutputs called so they forget about us.
    virtual bool deInitSystem() = 0;    // undo initSystem, as needed
    virtual bool runSystem(int microSeconds) = 0;   // run the system for a fixed amount of time

    // return how many cartridge ports the system has
    virtual int  hasCartridges() { return 0; }
    // load a cartridge to port from memory, create whatever needed, return false on failure
    // note: the memory is deleted after call, so make a copy of anything you need
    virtual bool loadCartridge(int port, const unsigned char *data) { (void)port; (void)data; return false; }
    // unload and destroy whatever cartridge was in port
    virtual bool unloadCartridge(int port) { (void)port; return false; }

    // Returns how many disk devices the system supports
    virtual int  hasDisks() { return 0; }
    // returns true if the device index supports configuration
    virtual bool hasDiskConfig(int disk) { (void)disk; return false; }
    // runs the configuration dialog for the specified disk
    // this can include changing disk devices, paths or images
    virtual void configureDisk(int disk) { (void)disk; }
    // returns true if the disk can be opened by the OS
    virtual bool hasDiskOpen(int disk) { (void)disk; return false; }
    // request the OS to open the disk, if possible
    virtual bool openDisk(int disk) { (void)disk; return false; }

    // additional optional peripherals - same as above except that
    // getPeripheralName returns a name for the menu for each optional
    // peripheral. This allows adding and removing hardware.
    virtual int  hasPeripherals() { return 0; }
    virtual const char* peripheralName(int periph) { (void)periph; return 0; }
    virtual bool loadPeripheral(int periph) { (void)periph; return false; }  // todo: may need data? multiple?
    virtual bool unloadPeripheral(int periph) { (void)periph; return false; }  // todo: may need data? multiple?
    virtual int  peripheralIOMask(int periph) { (void)periph; return PERIPHERAL_IO_NONE; }
    virtual bool inputData(int periph, int ioMask, const char *data) { (void)periph; (void)ioMask; (void)data; return false; }
    virtual bool mapOutput(int periph, int ioMask, Classic99System *pSys) { (void)periph; (void)ioMask; (void)pSys; return false; }
    virtual void unmapOutputs(Classic99System *pSys) { (void)pSys; }

    // the core (and others) may need access to this object's basic hardware
    // note these methods are allowed to return NULL if the hardware doesn't exist
    // each lock MUST be followed by an unlock, and you may NOT access the
    // returned pointer after unlocking.

    // note: we only support one VDP for now
    virtual Classic99VDP *lockVDP() {
        if (nullptr != mainVDP) {
            mainVDP->lock();
            return mainVDP;
        } else {
            return NULL;
        }
    }
    virtual void unlockVDP(Classic99VDP *vdp) {
        if (nullptr != vdp) {
            vdp->unlock();
        }
    }

    // note: support up to MAX_CPUS cpus
    virtual bool isCPU(int cpu) { return ((cpu < MAX_CPUS) && (mainCPU[cpu] != nullptr)); }
    virtual Classic99CPU *lockCPU(int cpu) {
        if (isCPU(cpu)) {
            mainCPU[cpu]->lock();
            return mainCPU[cpu];
        } else {
            return nullptr;
        }
    }
    virtual void unlockCPU(Classic99CPU *cpu) {
        if (cpu != nullptr) {
            cpu->unlock();
        }
    }

    // note: support up to MAX_PSGs PSGs
    virtual bool isPSG(int psg) { return ((psg < MAX_PSGS) && (mainPSG[psg] != nullptr)); }
    virtual Classic99PSG *lockPSG(int psg) {
        if (isPSG(psg)) {
            mainPSG[psg]->lock();
            return mainPSG[psg];
        } else {
            return nullptr;
        }
    }
    virtual void unlockPSG(Classic99PSG *psg) {
        if (psg != nullptr) {
            psg->unlock();
        }
    }

private:
    Classic99VDP *mainVDP;
    Classic99CPU *mainCPU[MAX_CPUS];
    Classic99PSG *mainPSG[MAX_PSGS];

};



#endif
