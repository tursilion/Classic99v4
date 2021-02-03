// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include "System.h"

// there is one global system object, eventually. This points to it.
Classic99System *theCore = NULL;

// base class implementation
Classic99System::Classic99System() {
    coreLock = al_create_mutex_recursive();

    // derived class MUST allocate these objects!
    // the single dummies at least prevent crashes
    memorySpaceRead = new PeripheralMap(&dummyPeripheral, 0);
    memorySpaceWrite = new PeripheralMap(&dummyPeripheral, 0);
    ioSpaceRead = new PeripheralMap(&dummyPeripheral, 0);
    ioSpaceWrite = new PeripheralMap(&dummyPeripheral, 0);
    memorySize = 0;
    ioSize = 0;
}

Classic99System::~Classic99System() {
    // release the lock object
    al_destroy_mutex(coreLock);
}

// handle allocating memory space in the general flat case
bool Classic99System::claimRead(int sysAdr, Classic99Peripheral *periph, int periphAdr) {
    if (sysAdr >= memorySize) return false;
    memorySpaceRead[sysAdr].updateMap(periph, periphAdr);
    return true;
}

bool Classic99System::claimWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr) {
    if (sysAdr >= memorySize) return false;
    memorySpaceWrite[sysAdr].updateMap(periph, periphAdr);
    return true;
}

bool Classic99System::claimIORead(int sysAdr, Classic99Peripheral *periph, int periphAdr) {
    if (sysAdr >= ioSize) return false;
    ioSpaceRead[sysAdr].updateMap(periph, periphAdr);
    return true;
}

bool Classic99System::claimIOWrite(int sysAdr, Classic99Peripheral *periph, int periphAdr) {
    if (sysAdr >= ioSize) return false;
    ioSpaceWrite[sysAdr].updateMap(periph, periphAdr);
    return true;
}

// read and write are normally handled here, at the system level,
// since most machines use a rather flat architecture.

// read from a memory address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
int Classic99System::readMemoryByte(int address, bool allowSideEffects) {
    if (address >= memorySize) {
        address &= memorySize;
    }
    return memorySpaceRead[address].who->read(memorySpaceRead[address].addr, false, allowSideEffects);
}

// write to a memory address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
// data - the byte of data being written
void Classic99System::writeMemoryByte(int address, bool allowSideEffects, int data) {
    if (address >= memorySize) {
        address &= memorySize;
    }
    memorySpaceWrite[address].who->write(memorySpaceWrite[address].addr, false, allowSideEffects, data);
}

// read from an IO address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
int Classic99System::readIOByte(int address, bool allowSideEffects) {
    if (address >= ioSize) {
        address &= ioSize;
    }
    return ioSpaceRead[address].who->read(ioSpaceRead[address].addr, true, allowSideEffects);
}

// write to an IO address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
// data - the byte of data being written
void Classic99System::writeIOByte(int address, bool allowSideEffects, int data) {
    if (address >= ioSize) {
        address &= ioSize;
    }
    ioSpaceWrite[address].who->write(ioSpaceWrite[address].addr, true, allowSideEffects, data);
}

// process any active debug systems, poll peripherals, etc...
// TODO
void Classic99System::processDebug() {
}

