// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class builds a TI-99/4 (1979) machine
// Later versions of the TI-99/4 subclass this one where needed
// as they are all very, very similar - only the keyboard, VDP
// and ROMs differ.

#include "TI994.h"

TI994::TI994()
    : Classic99System()
{
}

TI994::~TI994() {
}

bool TI994::initSystem() { 
    // derived class MUST allocate these objects!
    delete memorySpaceRead;
    delete memorySpaceWrite;
    delete ioSpaceRead;
    delete ioSpaceWrite;

    // CPU memory space is 64k
    memorySpaceRead = new PeripheralMap[64*1024];
    memorySpaceWrite = new PeripheralMap[64*1024];
    memorySize = 64*1024;

    // IO space is CRU on this machine, and is 4k in size...
    // Read and write functions often differ
    // TODO: including peripherals?
    ioSpaceRead = new PeripheralMap[4*1024];
    ioSpaceWrite = new PeripheralMap[4*1024];
    ioSize = 4*1024;

    // now create the peripherals we need
    theTV = new Classic99TV();
    theTV->init();
    pGrom = new TI994GROM(this);
    pGrom->init(0);
    pRom = new TI994ROM(this);
    pRom->init(0);
    pScratch = new TI994Scratchpad(this);
    pScratch->init(0);
    pVDP = new TMS9918(this);
    pVDP->init(0);

    // now we can claim resources
    for (int idx=0; idx<0x2000; ++idx) {
        claimRead(idx, pRom, idx);
    }
    for (int idx=0x8000; idx<0x8400; ++idx) {
        claimRead(idx, pScratch, idx&0xff);
        claimWrite(idx, pScratch, idx&0xff);
    }
    for (int idx=0x8800; idx<0x8c00; idx+=2) {
        claimRead(idx, pVDP, (idx&2) ? 1 : 0);
    }
    for (int idx=0x8c00; idx<0x9000; idx+=2) {
        claimWrite(idx, pVDP, (idx&2) ? 1 : 0);
    }
    for (int idx=0x9800; idx<0x9c00; idx+=2) {
        claimRead(idx, pGrom, (idx&2) ? Classic99GROM::GROM_MODE_ADDRESS : 0);
    }
    for (int idx=0x9c00; idx<0xa000; idx+=2) {
        claimWrite(idx, pGrom, (idx&2) ? (Classic99GROM::GROM_MODE_ADDRESS|Classic99GROM::GROM_MODE_WRITE) : Classic99GROM::GROM_MODE_WRITE);
    }

    // TODO Keyboard and the rest of the 9901
    // TODO sound

    // last, build and init the CPU (needs the memory map active!)
    pCPU = new TMS9900(this);
    pCPU->init(0);

    return true;
}

bool TI994::deInitSystem() {
    // unmap all the hardware
    ioSize = 0;
    memorySize = 0;

    // delete our arrays
    delete[] memorySpaceRead;
    delete[] memorySpaceWrite;
    delete[] ioSpaceRead;
    delete[] ioSpaceWrite;

    // zero the pointers
    memorySpaceRead = nullptr;
    memorySpaceWrite = nullptr;
    ioSpaceRead = nullptr;
    ioSpaceWrite = nullptr;

    // free the hardware
    delete pVDP;
    delete pRom;
    delete pGrom;
    delete pCPU;

    return true;
}

bool TI994::runSystem(int microSeconds) {
    currentTimestamp += microSeconds;

    // ROM and GROM don't need any runtime, so we can skip them
    pCPU->operate(currentTimestamp);
    pVDP->operate(currentTimestamp);

    return true;
}

