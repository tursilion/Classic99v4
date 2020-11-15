// Classic99 v4xx - Copyright 2020 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include "System.h"

Classic99System::Classic99System() {
    mainVDP = nullptr;
    for (int i=0; i<MAX_CPUS; ++i) {
        mainCPU[i] = nullptr;
    }
    for (int i=0; i<MAX_PSGS; ++i) {
        mainPSG[i] = nullptr;
    }

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

// note: we only support one VDP for now
Classic99VDP *Classic99System::lockVDP() {
    if (nullptr != mainVDP) {
        mainVDP->lock();
        return mainVDP;
    } else {
        return NULL;
    }
}
void Classic99System::unlockVDP(Classic99VDP *vdp) {
    if (nullptr != vdp) {
        vdp->unlock();
    }
}

Classic99CPU *Classic99System::lockCPU(int cpu) {
    if (isCPU(cpu)) {
        mainCPU[cpu]->lock();
        return mainCPU[cpu];
    } else {
        return nullptr;
    }
}
void Classic99System::unlockCPU(Classic99CPU *cpu) {
    if (cpu != nullptr) {
        cpu->unlock();
    }
}

Classic99PSG *Classic99System::lockPSG(int psg) {
    if (isPSG(psg)) {
        mainPSG[psg]->lock();
        return mainPSG[psg];
    } else {
        return nullptr;
    }
}
void Classic99System::unlockPSG(Classic99PSG *psg) {
    if (psg != nullptr) {
        psg->unlock();
    }
}

// read and write are normally handled here, at the system level,
// since most machines use a rather flat architecture. CPUs can
// decide what to do on their own, of course.

// read from a memory address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
int Classic99System::readMemoryByte(int address, bool allowSideEffects) {
    if (address >= memorySize) {
        address &= memorySize;
    }
    return memorySpaceRead[address].who->read(memorySpaceRead[address].addr, allowSideEffects);
}

// write to a memory address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
// data - the byte of data being written
void Classic99System::writeMemoryByte(int address, bool allowSideEffects, int data) {
    if (address >= memorySize) {
        address &= memorySize;
    }
    memorySpaceWrite[address].who->write(memorySpaceWrite[address].addr, allowSideEffects, data);
}

// read from an IO address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
int Classic99System::readIOByte(int address, bool allowSideEffects) {
    if (address >= ioSize) {
        address &= ioSize;
    }
    return ioSpaceRead[address].who->read(ioSpaceRead[address].addr, allowSideEffects);
}

// write to an IO address
// address - system address being accessed
// allowSideEffects - when false, this is a debugger access, do not trigger side effects
// data - the byte of data being written
void Classic99System::writeIOByte(int address, bool allowSideEffects, int data) {
    if (address >= ioSize) {
        address &= ioSize;
    }
    ioSpaceWrite[address].who->write(ioSpaceWrite[address].addr, allowSideEffects, data);
}

