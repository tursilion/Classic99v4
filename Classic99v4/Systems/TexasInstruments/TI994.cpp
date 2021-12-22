// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class builds a TI-99/4 (1979) machine
// Later versions of the TI-99/4 subclass this one where needed
// as they are all very, very similar - only the keyboard, VDP
// and ROMs differ.

#include "TI994.h"
#include "..\..\EmulatorSupport\interestingData.h"
#include "..\..\Memories\TI994GROM.h"
#include "..\..\Memories\TI994ROM.h"
#include "..\..\Keyboard\kb_994.h"

TI994::TI994()
    : Classic99System()
{
}

TI994::~TI994() {
}

// this virtual function is used to subclass the various types of TI99 - anything that varies is
// loaded and initialized in here.
bool TI994::initSpecificSystem() {
    pGrom = new TI994GROM(this);
    pGrom->init(0);
    pRom = new TI994ROM(this);
    pRom->init(0);
    pVDP = new TMS9918(this);
    pVDP->init(0);
    pKey = new KB994(this);
    pKey->init(0);

    return true;
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

    // set the interesting data
    setInterestingData(DATA_SYSTEM_TYPE, SYSTEM_TI99);
    setInterestingData(INDIRECT_MAIN_CPU_PC, DATA_TMS9900_PC);
    setInterestingData(INDIRECT_MAIN_CPU_INTERRUPTS_ENABLED, DATA_TMS9900_INTERRUPTS_ENABLED);

    // now create the peripherals we need
    theTV = new Classic99TV();
    theTV->init();
    pScratch = new TI994Scratchpad(this);
    pScratch->init(0);

    // call the virtual handler for the variants
    initSpecificSystem();

    // now we can claim resources
    const int ROMWait = 0;      // 0 wait states on ROM
    const int ScratchWait = 0;  // 0 wait states on scratchpad RAM
    const int VDPWait = 4;      // 4 wait states on VDP access
    const int GROMWait = 4;     // 4 wait states on GROM access
    const int KeyWait = 0;      // 0 wait states on keyboard access

    // system ROM
    for (int idx=0; idx<0x2000; ++idx) {
        claimRead(idx, pRom, idx, ROMWait);
    }

    // scratchpad RAM
    for (int idx=0x8000; idx<0x8400; ++idx) {
        claimRead(idx, pScratch, idx&0xff, ScratchWait);
        claimWrite(idx, pScratch, idx&0xff, ScratchWait);
    }

    // VDP ports
    for (int idx=0x8800; idx<0x8c00; idx+=2) {
        claimRead(idx, pVDP, (idx&2) ? 1 : 0, VDPWait);
    }
    for (int idx=0x8c00; idx<0x9000; idx+=2) {
        claimWrite(idx, pVDP, (idx&2) ? 1 : 0, VDPWait);
    }

    // GROM ports
    for (int idx=0x9800; idx<0x9c00; idx+=2) {
        claimRead(idx, pGrom, (idx&2) ? Classic99GROM::GROM_MODE_ADDRESS : 0, GROMWait);
    }
    for (int idx=0x9c00; idx<0xa000; idx+=2) {
        claimWrite(idx, pGrom, 
            (idx&2) ? (Classic99GROM::GROM_MODE_ADDRESS|Classic99GROM::GROM_MODE_WRITE) : Classic99GROM::GROM_MODE_WRITE, 
            GROMWait);
    }

    // IO ports for keyboard
    for (int idx=0; idx<0x800; idx+=20) {
        for (int off=3; off<=10; ++off) {
            claimIORead(idx+off, pKey, off, KeyWait);
        }
        for (int off=18; off<=20; ++off) {
            claimIOWrite(idx+off, pKey, off, KeyWait);
        }
        // I'm not sure what this is for, but the emulation seems to help
        // it not hang... (I hope). We just toggle this bit as a workaround -
        // the schematics say it's not even hooked up. (Though maybe it's 
        // related to timer mode, which I haven't coded yet. TODO.)
        // The 99/4 sometimes hangs on this bit... I suppose a disassembly
        // MIGHT clue in what's going on... Classic99 3xx does not have this issue.
        claimIORead(idx+17, pKey, 17, KeyWait);
    }

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
    delete pKey;
    delete pScratch;
    delete theTV;

    return true;
}

bool TI994::runSystem(int microSeconds) {
    currentTimestamp += microSeconds;

    // these guys don't need runtime, so not going to run them
    //pRom->operate(currentTimestamp);
    //pGrom->operate(currentTimestamp);
    //pScratch->operate(currentTimestamp);
    //pKey->operate(currentTimestamp);
    
    // these guys DO need runtime
    pCPU->operate(currentTimestamp);
    pVDP->operate(currentTimestamp);

    // route the interrupt lines
    if (pVDP->isIntActive()) {
        // TODO: emulate the masking on the 9901 - we may need a peripheral device
        requestInt(1);
    }

    return true;
}

