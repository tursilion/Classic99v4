// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include "System.h"
#include "peripheral.h"

// there is one global system object, eventually. This points to it.
Classic99System *theActiveCore = NULL;

// base class implementation
Classic99System::Classic99System() 
    : theTV(nullptr)
{
    coreLock = al_create_mutex_recursive();

    // derived class MUST allocate these objects!
    // the single dummies at least prevent crashes
    memorySpaceRead = new PeripheralMap(&dummyPeripheral, 0, 0);
    memorySpaceWrite = new PeripheralMap(&dummyPeripheral, 0, 0);
    ioSpaceRead = new PeripheralMap(&dummyPeripheral, 0, 0);
    ioSpaceWrite = new PeripheralMap(&dummyPeripheral, 0, 0);
    memorySize = 0;
    ioSize = 0;
    currentTimestamp = 0.0;
}

Classic99System::~Classic99System() {
    // release the lock object
    al_destroy_mutex(coreLock);
    
    // delete the dummy objects
    if (nullptr != memorySpaceRead) delete memorySpaceRead;
    if (nullptr != memorySpaceWrite) delete memorySpaceWrite;
    if (nullptr != ioSpaceRead) delete ioSpaceRead;
    if (nullptr != ioSpaceWrite) delete ioSpaceWrite;
}

// handle allocating memory space in the general flat case
bool Classic99System::claimRead(int sysAdr, Classic99Peripheral *periph, int periphAdr, int waitStates) {
    if (sysAdr >= memorySize) return false;
    memorySpaceRead[sysAdr].updateMap(periph, periphAdr, waitStates);
    return true;
}

bool Classic99System::claimWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr, int waitStates) {
    if (sysAdr >= memorySize) return false;
    memorySpaceWrite[sysAdr].updateMap(periph, periphAdr, waitStates);
    return true;
}

bool Classic99System::claimIORead(int sysAdr, Classic99Peripheral *periph, int periphAdr, int waitStates) {
    if (sysAdr >= ioSize) return false;
    ioSpaceRead[sysAdr].updateMap(periph, periphAdr, waitStates);
    return true;
}

bool Classic99System::claimIOWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr, int waitStates) {
    if (sysAdr >= ioSize) return false;
    ioSpaceWrite[sysAdr].updateMap(periph, periphAdr, waitStates);
    return true;
}

// read and write are normally handled here, at the system level,
// since most machines use a rather flat architecture.

// read from a memory address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
uint8_t Classic99System::readMemoryByte(int address, volatile long &cycles, MEMACCESSTYPE rmw) {
    if (address >= memorySize) {
        address &= memorySize;
    }
    if ((rmw&ACCESS_SYSTEM) == 0) {
        cycles+=memorySpaceRead[address].waitStates;
    }
    rmw = static_cast<MEMACCESSTYPE>(((int)rmw)&(ACCESS_SYSTEM-1));
    return memorySpaceRead[address].who->read(memorySpaceRead[address].addr, false, cycles, rmw);
}

// write to a memory address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
// data - the byte of data being written
void Classic99System::writeMemoryByte(int address, volatile long &cycles, MEMACCESSTYPE rmw, int data) {
    if (address >= memorySize) {
        address &= memorySize;
    }
    if ((rmw&ACCESS_SYSTEM) == 0) {
        cycles+=memorySpaceRead[address].waitStates;
    }
    rmw = static_cast<MEMACCESSTYPE>(((int)rmw)&(ACCESS_SYSTEM-1));
    memorySpaceWrite[address].who->write(memorySpaceWrite[address].addr, false, cycles, rmw, data);
}

// read from an IO address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
uint8_t Classic99System::readIOByte(int address, volatile long &cycles, MEMACCESSTYPE rmw) {
    if (address >= ioSize) {
        address &= ioSize;
    }
    if ((rmw&ACCESS_SYSTEM) == 0) {
        cycles+=memorySpaceRead[address].waitStates;
    }
    rmw = static_cast<MEMACCESSTYPE>(((int)rmw)&(ACCESS_SYSTEM-1));
    return ioSpaceRead[address].who->read(ioSpaceRead[address].addr, true, cycles, rmw);
}

// write to an IO address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
// data - the byte of data being written
void Classic99System::writeIOByte(int address, volatile long &cycles, MEMACCESSTYPE rmw, int data) {
    if (address >= ioSize) {
        address &= ioSize;
    }
    if ((rmw&ACCESS_SYSTEM) == 0) {
        cycles+=memorySpaceRead[address].waitStates;
    }
    rmw = static_cast<MEMACCESSTYPE>(((int)rmw)&(ACCESS_SYSTEM-1));
    ioSpaceWrite[address].who->write(ioSpaceWrite[address].addr, true, cycles, rmw, data);
}

// cause the system to stop for breakpoint
// TODO
void Classic99System::triggerBreakpoint() {
}

// process any active debug systems, poll peripherals, etc...
// TODO
void Classic99System::processDebug() {
}

