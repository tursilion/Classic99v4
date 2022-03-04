// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)
//
// Implementation of TMS9900 - written by Mike Brent
//
/////////////////////////////////////////////////////////////////////
// Classic99 - TMS9900 CPU Routines
// M.Brent
// The TMS9900 is a 16-bit CPU by Texas Instruments, with a 16
// bit data and 16-bit address path, capable of addressing
// 64k of memory. All reads and writes are word (16-bit) oriented.
// Byte access is simulated within the CPU by reading or writing
// the entire word, and manipulating only the requested byte.
// The CPU uses external RAM for all user registers. 
// There are 16 user registers, R0-R15, and the memory base for 
// these registers may be anywhere in memory, set by the Workspace 
// Pointer. The CPU also has a Program Counter and STatus register 
// internal to it.
// This emulation generates a lookup table of addresses for each
// opcode. It's not currently intended for use outside of Classic99
// and there may be problems with dependancy on other parts of the
// code if it is moved to another project.
/////////////////////////////////////////////////////////////////////

// TODO implement the Classic99 debug opcodes 0x0110 through 0x0113

#include <cstdio>
#include "../EmulatorSupport/peripheral.h"
#include "../EmulatorSupport/debuglog.h"
#include "../EmulatorSupport/System.h"
#include "../EmulatorSupport/interestingData.h"
#include "tms9900.h"

#ifndef BUILD_CPU
extern const CPU9900Fctn opcode[65536];				// CPU Opcode address table
extern const Word WStatusLookup[64*1024];           // word statuses
extern const Word BStatusLookup[256];               // byte statuses
#endif

// protect the disassembly backtrace
ALLEGRO_MUTEX *csDisasm;

// System interface

TMS9900::TMS9900(Classic99System *core) : Classic99Peripheral(core) {
}

TMS9900::~TMS9900() {
}

// claim resources from the core system, prepare the CPU
bool TMS9900::init(int idx) {
    setIndex("TMS9900", idx);
#ifdef BUILD_CPU
    buildcpu();
#endif
    reset();

    return true;
}

// process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
bool TMS9900::operate(double timestamp) {
    // handle first call or timestamp wraparound
    if ((lastTimestamp == 0) || (timestamp < lastTimestamp)) {
        lastTimestamp = timestamp;
        return true;
    }

    // are we stopped? If so, there is nothing we can do internally to release
    if (theCore->getHoldStatus(-1)) {
        lastTimestamp = timestamp;
        return true;
    }

    // we might still be idling, but that would be released by an interrupt
    if (!theCore->interruptPending()) {
        if (GetIdle()) {
            lastTimestamp = timestamp;
            return true;
        }
    }

    // run as many cycles as have elapsed
    ResetCycleCount();
    // CPU runs at 3MHz, so each CPU cycle is 1/3 of a timestamp time
    for ( ; lastTimestamp < timestamp; lastTimestamp += (GetCycleCount()*(1.0/3.0))) {
        // prepare to count cycles this instruction
        ResetCycleCount();

        // check for interrupts. I think technically this
        // goes after each instruction, but except for the
        // very first one, what's the difference?
        // (Thus the countdown on skip_interrupt)
        if (skip_interrupt > 0) {
            --skip_interrupt;
        } else {
            if (theCore->interruptPending()) {
                if (theCore->getNMI()) {
                    TriggerInterrupt(-1);
                    // now continue so the cycles are tracked
                    continue;
                } else {
                    // must be a level interrupt. Although the 99/4A only has one
                    // level, we'll be a good 9900 and check them all...
                    int minLevel = GetST()&0x000f;  // get the level mask, ints must be equal or lower
                    uint32_t ints = theCore->getIntLevels();
                    for (int idx = 0; idx < minLevel; ++idx) {
                        if (ints & (1<<idx)) {
                            // all ints are wired to level 1 on the TI, so the vector is 4
                            // the convention to LIMI 2 actually enables levels 1 and 2, but
                            // only level 1 is wired. This is frequently confusing so documented now.
                            // TODO: this implies that on the 99/4A, a VDP interrupt can not
                            // interrupt another VDP interrupt, as the mask is automatically
                            // set to zero from the level 1 interrupt. The datasheet confirms
                            // that this is the intent, meaning LIMI 0 does NOT need to be
                            // the first instruction to be safe. But the hard-coded ROM has
                            // that, so how can we test this on real hardware? We just need
                            // a routine that dumps the status register...
                            // There is another gotcha - this needs to run through the 9901...
                            TriggerInterrupt(idx);
                            minLevel = -1;
                            break;
                        }
                    }
                    // there may be another interrupt available, but it's not permitted, so loop around
                    // to count the cycles and proceed
                    if (minLevel == -1) {
                        continue;
                    }
                }
            }
        }

        // now actually execute the instruction
        in = ROMWORD(PC);       // "in" is also used by the opcodes!!
        ADDPC(2);
        CALL_MEMBER_FN(this, opcode[in])();
    }

    // ran successfully
    return true;
}

// release everything claimed in init, save NV data, etc
bool TMS9900::cleanup() {
    // actually, nothing to do here
    return true;
}

// dimensions of a text mode output screen - either being 0 means none
void TMS9900::getDebugSize(int &x, int &y) {
    // TODO: Just going to use the existing version for now
    x = 32;
    y = 15;
}

// output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
void TMS9900::getDebugWindow(char *buffer) {
    // the buffer is guaranteed to be 32x30 with extra space for line endings
    // For all the stuff that's "missing", it's okay for the /debug system/ to composite
    // it, but there's no reason for the CPU to know about other chips...

    // prints the register information
    // spacing: <5 label><1 space><4 value><4 spaces><5 label><1 space><4 value>
    int WP = GetWP();
    for (int idx=0; idx<8; idx++) {
	    int val=ROMWORD(WP+idx*2, ACCESS_FREE);
	    int val2=ROMWORD(WP+(idx+8)*2, ACCESS_FREE);
        if (idx == 0) {
    	    buffer += sprintf(buffer, " R%2d  %04X   R%2d  %04X\r\n", idx, val, idx+8, val2);
        } else {
    	    buffer += sprintf(buffer, " R%2d  %04X   R%2d  %04X\r\n", idx, val, idx+8, val2);
        }
    }

    buffer += sprintf(buffer, "\r\n");

    buffer += sprintf(buffer, "  PC  %04X\r\n", GetPC());
    buffer += sprintf(buffer, "  WP  %04X\r\n", GetWP());
    buffer += sprintf(buffer, "  ST  %04X\r\n", GetST());

    buffer += sprintf(buffer, "\r\n");

    // break down the status register
    int val=GetST();
    buffer += sprintf(buffer, "  ST : %s %s %s %s %s %s %s\r\n", 
        (val&BIT_LGT)?"LGT":"   ", 
        (val&BIT_AGT)?"AGT":"   ", 
        (val&BIT_EQ)?"EQ":"  ",
	    (val&BIT_C)?"C":" ", 
        (val&BIT_OV)?"OV":"  ", 
        (val&BIT_OP)?"OP":"  ", 
        (val&BIT_XOP)?"XOP":" "
    );
    buffer += sprintf(buffer, " MASK: %X\r\n", val&ST_INTMASK);

#ifdef _DEBUG
    // TODO: move this to the code that calls getDebugWindow, so the overflow test is centralized
    // Instead of a buffer size check, we should put some poison values after the buffer and just
    // check if they are overwritten. 4 bytes is just one int.
    int x,y;
    getDebugSize(x,y);
    if (buffer > buffer+(x+2)*y) {
        debug_write("BUFFER OVERFLOW IN CPU DEBUG");
    }
#endif
}

// number of bytes needed to save state
// You can grow a buffer, but you can never shrink it.
// Try to make save states as compatible as possible when you do.
int TMS9900::saveStateSize() {
    return 5*4;
}

// write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
bool TMS9900::saveState(unsigned char *buffer) {
    // Write out the data we need to save
    saveStateVal(buffer, PC);
    saveStateVal(buffer, WP);
    saveStateVal(buffer, ST);
    saveStateVal(buffer, idling);
    saveStateVal(buffer, skip_interrupt);

    return true;
}

// restore state data from the provided buffer - guaranteed to be AT LEAST the size returned by saveStateSize
bool TMS9900::restoreState(unsigned char *buffer) {
    loadStateVal(buffer, PC);
    loadStateVal(buffer, WP);
    loadStateVal(buffer, ST);
    loadStateVal(buffer, idling);
    loadStateVal(buffer, skip_interrupt);

    return true;
}

// Note: Post-increment is a trickier case that it looks at first glance.
// For operations like MOV R3,*R3+ (from Corcomp's memory test), the address
// value is written to the same address before the increment occurs.
// There are even trickier cases in the console like MOV *R3+,@>0008(R3),
// where the post-increment happens before the destination address calculation.
// Then in 2021 we found MOV *R0+,R1, where R0 points to itself, and found
// that the increment happens before *R0 is fetched for the move.
// Thus it appears the steps need to happen in this order:
//
// 1) Calculate source address
// 2) Handle source post-increment
// 3) Get Source data
// 4) Calculate destination address
// 5) Handle Destination post-increment
// 6) Store destination data
//
// Only the following instruction formats support post-increment:
// FormatI
// FormatIII (src only) (destination can not post-increment)
// FormatIV (src only) (has no destination)
// FormatVI (src only) (has no destination)
// FormatIX (src only) (has no destination)

/////////////////////////////////////////////////////////////////////
// Inlines because for the most part, the opcodes are the same
/////////////////////////////////////////////////////////////////////
// theOp - operation to perform
// Assumes xD is the word data var, and x2 is the byte data var, 
// x3 is the byte result, and D is the address
// use is like: xD=ROMWORD(adr); BYTE_OPERATION(x3=x2-x1); WRWORD(D,xD);
// This is used when the RMW cycle /actually does/ a Modify ;)
#define BYTE_OPERATION(theOp) \
    if (D&1) {                      \
        x2 = (xD&0xff); /*LSB*/     \
        theOp;                      \
        x3 &= 0xff;                 \
        xD = (xD&0xff00) | x3;      \
    } else {                        \
        x2 = (xD>>8);  /*MSB*/      \
        theOp;                      \
        x3 &= 0xff;                 \
        xD = (xD&0xff) | (x3<<8);   \
    }

// FormatI is most of the basic Add/subtract/move/compare operations
// x1 contains the source value, x2 the destination, x3 the result
// (except operations with no ALU, like move).
// theOp should calculate x3 from x1 and x2.
// These all take 14 cycles
#define FormatI(theOp)          \
    Word x1, x2, x3;            \
    AddCycleCount(14);          \
                                \
    Td=(in&0x0c00)>>10;         \
    Ts=(in&0x0030)>>4;          \
    D=(in&0x03c0)>>6;           \
    S=(in&0x000f);              \
    B=(in&0x1000)>>12;          \
    fixS();                     \
                                \
    x1 = ROMWORD(S);            \
    fixD();                     \
    x2 = ROMWORD(D);            \
    theOp;


// This is FormatI for byte operations
// theOp should return x3 from x1 and x2
// xD is used internally as the destination word
#define FormatIb(theOp)         \
    Byte x1, x2, x3;            \
    Word xD;                    \
    AddCycleCount(14);          \
                                \
    Td=(in&0x0c00)>>10;         \
    Ts=(in&0x0030)>>4;          \
    D=(in&0x03c0)>>6;           \
    S=(in&0x000f);              \
    B=(in&0x1000)>>12;          \
    fixS();                     \
                                \
    x1 = RCPUBYTE(S);           \
    fixD();                     \
    xD = ROMWORD(D);            \
    BYTE_OPERATION(theOp);

// The base FormatII covers CRU bit operations, but here
// we just calculate the CRU address in 'add'
#define FormatII                \
    Word add;                   \
                                \
    D=(in&0x00ff);              \
    AddCycleCount(12);          \
                                \
    add=(ROMWORD(WP+24)>>1);    \
    if (D&0x80) {               \
        add-=128-(D&0x7f);      \
    } else {                    \
        add+=D;                 \
    }

// FormatII also covers jump instructions, here we
// perform the entire jump - cond is the condition to test
#define FormatIIj(cond)         \
    D=(in&0x00ff);              \
                                \
    if (cond) {                 \
        if (X_flag) {           \
            SetPC(X_flag);      \
        }                       \
        if (D&0x80) {           \
            D=128-(D&0x7f);     \
            ADDPC(-(D+D));      \
        } else {                \
            ADDPC(D+D);         \
        }                       \
        AddCycleCount(10);      \
    } else {                    \
        AddCycleCount(8);       \
    }

// FormatIII covers word-only boolean operations
// theOp should set x3 from x1 and x2, as above
#define FormatIII(theOp)        \
    Word x1,x2,x3;              \
                                \
    Td=0;                       \
    Ts=(in&0x0030)>>4;          \
    D=(in&0x03c0)>>6;           \
    S=(in&0x000f);              \
    B=0;                        \
    fixS();                     \
                                \
    AddCycleCount(14);          \
                                \
    x1 = ROMWORD(S);            \
    fixD();                     \
    x2 = ROMWORD(D);            \
    theOp;

// FormatIV is just LDCR and STCR, and they have nothing
// in common beyond this bit packing...
#define FormatIV                \
    D=(in&0x03c0)>>6;           \
    Ts=(in&0x0030)>>4;          \
    S=(in&0x000f);              \
    B=(D<9);                    \
    fixS();

// FormatV is shift instructions
// Source value is returned in x1,
// shift count in D, the rest is up to you
#define FormatV                 \
    Word x1;                    \
                                \
    D=(in&0x00f0)>>4;           \
    S=(in&0x000f);              \
    S=WP+(S<<1);                \
                                \
    if (D==0) {                 \
        D=ROMWORD(WP) & 0xf;    \
        if (D==0) D=16;         \
        AddCycleCount(8);       \
    }                           \
    AddCycleCount(12+2*D);      \
    x1=ROMWORD(S);

// The base FormatVI is math functions, SETO, CLR, etc
// There's no D, S is used for read and write
// theOp should set x2 based on x1
#define FormatVI(theOp)         \
    Ts=(in&0x0030)>>4;          \
    S=in&0x000f;                \
    B=0;                        \
    fixS();                     \
                                \
    Word x1,x3;                 \
    AddCycleCount(10);          \
    x1 = ROMWORD(S);            \
    (void)x1;                   \
    theOp;                      \
    WRWORD(S, x3);

// FormatVIraw is used for branch instructions, and ABS
// It returns the source value in x1
#define FormatVIraw             \
    Ts=(in&0x0030)>>4;          \
    S=in&0x000f;                \
    B=0;                        \
    fixS();                     \
                                \
    Word x1;                    \
    x1 = ROMWORD(S);            \

// FormatVII has no arguments and not much in common
#define FormatVII { }

// FormatVIII_0 is used to store internal registers
// pass the CPU register to store
#define FormatVIII_0(reg)       \
    D=(in&0x000f);              \
    D=WP+(D<<1);                \
                                \
    AddCycleCount(8);           \
    WRWORD(D, reg);

// FormatVIII_1 is used for immediate operand operations
// theOp should set x3 from x1 and x2
#define FormatVIII_1(theOp)     \
    Word x1,x2,x3;              \
                                \
    D=(in&0x000f);              \
    D=WP+(D<<1);                \
    S=ROMWORD(PC);              \
    ADDPC(2);                   \
                                \
    AddCycleCount(14);          \
    x1 = ROMWORD(D);            \
    x2 = S;                     \
    theOp;                      \
    WRWORD(D, x3);

// FormatVIII_1raw is used for non-logic ops, like LI
#define FormatVIII_1raw         \
    D=(in&0x000f);              \
    D=WP+(D<<1);                \
    S=ROMWORD(PC);              \
    ADDPC(2);

// FormatIXraw is used for DIV, MPY and XOP
#define FormatIXraw             \
    D=(in&0x03c0)>>6;           \
    Ts=(in&0x0030)>>4;          \
    S=(in&0x000f);              \
    B=0;                        \
    fixS();


//////////////////////////////////////////////////////////////////////////
// Get addresses for the destination and source arguments
// Note: the format code letters are the official notation from Texas
// instruments. See their TMS9900 documentation for details.
// (Td, Ts, D, S, B, etc)
// Note that some format codes set the destination type (Td) to
// '4' in order to skip unneeded processing of the Destination address
//////////////////////////////////////////////////////////////////////////
void TMS9900::fixS()
{
    int temp,t2;                                                    // temp vars

    switch (Ts)                                                     // source type
    { 
    case 0: S=WP+(S<<1);                                            // 0 extra memory cycles
            break;                                                  // register                     (R1)            Address is the address of the register

    case 1: 
            S=ROMWORD(WP+(S<<1));                                   // 1 extra memory cycle: read register
            AddCycleCount(4); 
            break;                                                  // register indirect            (*R1)           Address is the contents of the register

    case 2: 
            if (S) {                                                // 2 extra memory cycles: read register, read argument
                S=ROMWORD(PC)+ROMWORD(WP+(S<<1));                   // indexed                      (@>1000(R1))    Address is the contents of the argument plus the
            } else {                                                //                                              contents of the register
                S=ROMWORD(PC);                                      // 1 extra memory cycle: read argument
            }                                                       // symbolic                     (@>1000)        Address is the contents of the argument
            ADDPC(2); 
            AddCycleCount(8);
            break;

    case 3: 
            t2=WP+(S<<1);                                           // 2 extra memory cycles: read register, write register
            temp=ROMWORD(t2); 
            S=temp;
            // After we have the final address, we can increment the register (so MOV *R0+ returns the post increment if R0=adr(R0))
            WRWORD(t2, temp + (B == 1 ? 1:2));                      // do count this write
            AddCycleCount((B==1?6:8));                              // (add 1 if byte, 2 if word)   (*R1+)          Address is the contents of the register, which
            break;                                                  // register indirect autoincrement              is incremented by 1 for byte or 2 for word ops
    }
}

void TMS9900::fixD()
{
    int temp,t2;                                                    // temp vars

    switch (Td)                                                     // destination type 
    {                                                               // same as the source types
    case 0: 
            D=WP+(D<<1);                                            // 0 extra memory cycles
            break;                                                  // register

    case 1: D=ROMWORD(WP+(D<<1));                                   // 1 extra memory cycle: read register 
            AddCycleCount(4);
            break;                                                  // register indirect

    case 2: 
            if (D) {                                                // 2 extra memory cycles: read register, read argument 
                D=ROMWORD(PC)+ROMWORD(WP+(D<<1));                   // indexed 
            } else {
                D=ROMWORD(PC);                                      // 1 extra memory cycle: read argument
            }                                                       // symbolic
            ADDPC(2);
            AddCycleCount(8);
            break;

    case 3: 
            t2=WP+(D<<1);                                           // 2 extra memory cycles: read register, write register
            temp=ROMWORD(t2);
            D=temp; 
            // After we have the final address, we can increment the register (so MOV *R0+ returns the post increment if R0=adr(R0))
            WRWORD(t2, temp + (B == 1 ? 1:2));                      // do count this write - add 1 if byte, 2 if word
            AddCycleCount((B==1?6:8)); 
            break;                                                  // register indirect autoincrement
    }
}

void TMS9900::reset() {
    // Base cycles: 26
    // 5 memory accesses:
    //  Read WP
    //  Write WP->R13
    //  Write PC->R14
    //  Write ST->R15
    //  Read PC
    
    // TODO: Read WP & PC are obvious, what are the other 3? Does it do a TRUE BLWP? Can we see where a reset came from?
    // It matches the LOAD interrupt, so maybe yes! Test above theory on hardware.
    
    StopIdle();
    nReturnAddress=0;

    TriggerInterrupt(0);                    // reset vector is 22 cycles
    AddCycleCount(4);                       // reset is slightly more work than a normal interrupt

    X_flag=0;                               // not currently executing an X instruction
    SetST(ST&0xfff0);                       // disable interrupts

// TODO: saving this so I remember it's needed for the F18A version
//    spi_reset();                            // reset the F18A flash interface
}

/////////////////////////////////////////////////////////////////////
// Wrapper functions for memory access
/////////////////////////////////////////////////////////////////////
Byte TMS9900::RCPUBYTE(Word src) {
    // TMS9900 always reads a full word, and in the case where
    // the multiplexer on the TI is invoked, it's always LSB first.
    // So, there is a little bit of system knowledge here because
    // the rest of Classic99 is built as an 8-bit machine. Hopefully
    // there are no cases where it will matter.

    // always read both bytes
    Byte lsb = theCore->readMemoryByte(src|1, nCycleCount, ACCESS_READ);         // odd byte
    Byte msb = theCore->readMemoryByte(src&0xfffe, nCycleCount, ACCESS_READ);    // even byte

    if (src&1) {
        return lsb;
    } else {
        return msb;
    }
}

void TMS9900::WCPUBYTE(Word dest, Byte c) {
    // TMS9900 always writes a full word, and always reads before the write -
    // we have no choice here. We do the same 8-bit fakery as noted above.

    // always read both bytes
    int adr = dest & 0xfffe;    // make even, but remember the original value
    Byte lsb = theCore->readMemoryByte(adr+1, nCycleCount, ACCESS_RMW);   // odd byte
    Byte msb = theCore->readMemoryByte(adr, nCycleCount, ACCESS_RMW);    // even byte

    // always write both bytes
    if (dest&1) {
        theCore->writeMemoryByte(adr+1, nCycleCount, ACCESS_WRITE, c);
        theCore->writeMemoryByte(adr, nCycleCount, ACCESS_WRITE, msb);

        if (adr == 0x8356) setInterestingData(DATA_TMS9900_8356, msb*256+c);
        else if (adr == 0x8370) setInterestingData(DATA_TMS9900_8370, msb*256+c);
    } else {
        theCore->writeMemoryByte(adr+1, nCycleCount, ACCESS_WRITE, lsb);
        theCore->writeMemoryByte(adr, nCycleCount, ACCESS_WRITE, c);

        if (adr == 0x8356) setInterestingData(DATA_TMS9900_8356, c*256+lsb);
        else if (adr == 0x8370) setInterestingData(DATA_TMS9900_8370, c*256+lsb);
    }
}

int TMS9900::ROMWORD(Word src, MEMACCESSTYPE rmw) {
    // most common basic access
    // as above, read lsb first in a bit of 99/4A specific knowledge
    // the real TMS9900 is 16-bit only, but Classic99's architecture is 8 bit
    Byte lsb = theCore->readMemoryByte(src|1, nCycleCount, rmw);                // odd byte
    Byte msb = theCore->readMemoryByte(src&0xfffe, nCycleCount, rmw);   // even byte
    return (msb<<8)|lsb;
}

void TMS9900::WRWORD(Word dest, Word val) {
    // Don't read-before-write in here - do that explicitly when you need it!
    dest &= 0xfffe;     // make even, we don't need the original value

    // now write the new data, in 99/4A order
    theCore->writeMemoryByte(dest+1, nCycleCount, ACCESS_WRITE, val&0xff);
    theCore->writeMemoryByte(dest, nCycleCount, ACCESS_WRITE, (val>>8)&0xff);

    if (dest == 0x8356) setInterestingData(DATA_TMS9900_8356, val);
    else if (dest == 0x8370) setInterestingData(DATA_TMS9900_8370, val);
}

// Helpers for what used to be global variables
void TMS9900::StartIdle() {
    idling = 1;
}
void TMS9900::StopIdle() {
    idling = 0;
}
int  TMS9900::GetIdle() {
    return idling;
}

void TMS9900::SetReturnAddress(Word x) {
    nReturnAddress = x;
}
Word TMS9900::GetReturnAddress() {
    return nReturnAddress;
}
void TMS9900::ResetCycleCount() {
    nCycleCount = 0;
}
void TMS9900::AddCycleCount(int val) {
    nCycleCount += val;
}
int TMS9900::GetCycleCount() {
    return nCycleCount;
}

// interrupt handling
void TMS9900::TriggerInterrupt(int level) {
    // Base cycles: 22
    // 5 memory accesses:
    //  Read WP
    //  Write WP->R13
    //  Write PC->R14
    //  Write ST->R15
    //  Read PC

    // -1 is the LOAD vector
    // Datasheet confirms the mask is set to 0 for LOAD
    Word vector = (level*4);

    // debug helper if logging
    // TODO: the debug disassembly log needs to be integrated into the debug system
    // we can just make the function call and let it decide whether to log or discard it
#if 0
    EnterCriticalSection(&csDisasm);
        if (NULL != fpDisasm) {
            if ((disasmLogType == 0) || (pCurrentCPU->GetPC() > 0x2000)) {
                fprintf(fpDisasm, "**** Interrupt Trigger, vector >%04X (%s), level %d\n",
                    vector, 
                    (vector==0)?"reset":(vector==4)?"console":(vector=0xfffc)?"load":"unknown",
                    level);
            }
        }
    LeaveCriticalSection(&csDisasm);
#endif

    // no more idling!
    StopIdle();
    
    // set up the new context
    int NewWP = ROMWORD(vector);

    WRWORD(NewWP+26,WP);                // WP in new R13 
    WRWORD(NewWP+28,PC);                // PC in new R14 
    WRWORD(NewWP+30,ST);                // ST in new R15 

    // lower the interrupt level
    if (level <= 0) {
        SetST(ST&0xfff0);
    } else {
        SetST((ST&0xfff0) | (level-1));
    }

    int NewPC = ROMWORD(vector+2);

    /* now load the correct workspace, and perform a branch and link to the address */
    SetWP(NewWP);
    SetPC(NewPC);

    AddCycleCount(22);
    SET_SKIP_INTERRUPT;                 // you get one instruction to turn interrupts back off
                                        // this is true for all BLWP-like operations
}
Word TMS9900::GetPC() {
    return PC;
}

// should rarely be externally used (Classic99 uses it for disk emulation)
void TMS9900::SetPC(Word x) {
    if (x&0x0001) {
        debug_write("Warning: setting odd PC address from >%04X", PC);
    }

    // the PC is 15 bits wide - confirmed via BigFoot game which relies on this
    PC=x&0xfffe;

    // TODO: the F18A GPU derivative should not set this!
    setInterestingData(DATA_TMS9900_PC, PC);
}

Word TMS9900::GetST() {
    return ST;
}
void TMS9900::SetST(Word x) {
    ST=x;

    // we assume that TMS9900 systems are single-cpu
    // TODO: the F18A GPU derivative should not set this!
    setInterestingData(DATA_TMS9900_INTERRUPTS_ENABLED, (x & 0x0f) ? DATA_TRUE : DATA_FALSE);
}
Word TMS9900::GetWP() {
    return WP;
}
void TMS9900::SetWP(Word x) {
    // TODO: confirm on hardware - is the WP also 15-bit?
    // we can test using BLWP and see what gets stored
    WP=x&0xfffe;

    // TODO: the F18A GPU derivative should not set this!
    setInterestingData(DATA_TMS9900_WP, WP);
}

////////////////////////////////////////////////////////////////////
// Classic99 - 9900 CPU opcodes
// Opcode functions follow
// one function for each opcode (the mneumonic prefixed with "op_")
// src - source address (register or memory - valid types vary)
// dst - destination address
// imm - immediate value
// dsp - relative displacement
////////////////////////////////////////////////////////////////////

void TMS9900::op_a()
{
    // Add words: A src, dst

    // In case I care in the future, every machine step is 2 cycles -- so a four cycle memory
    // access (which is indeed normal) is 2 cycles on the bus, and 2 cycles of thinking

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatI(x3=x2+x1);	// Read Source, Read Dest
    WRWORD(D,x3);       // Write Dest

    reset_EQ_LGT_AGT_C_OV;
    ST |= WStatusLookup[x3&0xffff]&mask_LGT_AGT_EQ;
    if (x3<x2) set_C;                                                                   // if it wrapped around, set carry
    if (((x1&0x8000)==(x2&0x8000))&&((x3&0x8000)!=(x2&0x8000))) set_OV;                 // if it overflowed or underflowed (signed math), set overflow
}

void TMS9900::op_ab()
{ 
    // Add bytes: A src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatIb(x3=x2+x1);	// Read Source, Read Dest
    WRWORD(D,xD);       // write dest

    reset_EQ_LGT_AGT_C_OV_OP;
    ST|=BStatusLookup[x3]&mask_LGT_AGT_EQ_OP;

    if (x3<x2) set_C;
    if (((x1&0x80)==(x2&0x80))&&((x3&0x80)!=(x2&0x80))) set_OV;
}

void TMS9900::op_abs()
{ 
    // ABSolute value: ABS src
    
    // MSB == 0
    // Base cycles: 12
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read source

    // MSB == 1
    // Base cycles: 14
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    Word x2;

    FormatVIraw;

    if (x1&0x8000) {
        x2=(~x1)+1;     // if negative, make positive
        WRWORD(S,x2);   // write source
        AddCycleCount(14);
    } else {
        AddCycleCount(12);
    }

    reset_EQ_LGT_AGT_C_OV;
    ST|=WStatusLookup[x1]&mask_LGT_AGT_EQ_OV;
}

void TMS9900::op_ai()
{ 
    // Add Immediate: AI src, imm
    
    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatVIII_1(x3=x1+x2);

    reset_EQ_LGT_AGT_C_OV;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;

    if (x3<x1) set_C;
    if (((x1&0x8000)==(S&0x8000))&&((x3&0x8000)!=(S&0x8000))) set_OV;
}

void TMS9900::op_dec()
{ 
    // DECrement: DEC src

    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=x1-1);

    reset_EQ_LGT_AGT_C_OV;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;

    if (x3!=0xffff) set_C;
    if (x3==0x7fff) set_OV;
}

void TMS9900::op_dect()
{ 
    // DECrement by Two: DECT src
    
    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=x1-2);
    
    reset_EQ_LGT_AGT_C_OV;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;

    if (x3<0xfffe) set_C;
    if ((x3==0x7fff)||(x3==0x7ffe)) set_OV;
}

void TMS9900::op_div()
{ 
    // DIVide: DIV src, dst
    // Dest, a 2 word number, is divided by src. The result is stored as two words at the dst:
    // the first is the whole number result, the second is the remainder

    // ST4 (OV) is to be set:
    // Base cycles: 16
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source MSW
    //  Read dest

    // ST4 (OV) is not to be set:
    // Base cycles: 92 - 124
    // 6 memory accesses:
    //  Read instruction (already done)
    //  Read source MSW
    //  Read dest
    //  Read source LSW
    //  Write dest quotient
    //  Write dest remainder

    // Sussing out the cycle count. It is likely a shift and test
    // approach, due to the 16 bit cycle variance. We know the divisor
    // is larger than the most significant word, ie: the total output
    // of the first 16 bits of the 32-bit result MUST be >0000, so
    // the algorithm likely starts with the first bit of the LSW, and
    // shifts through up to 16 cycles, aborting early if the remaining
    // value is smaller than the divisor (or if it's zero, but that
    // would early out in far fewer cases).
    //
    // For example (using 4 bits / 2 bits = 2 bits):
    //  DDDD
    // / VV
    //  ---
    //    1 (subtract from DDDx if set)
    //
    //  0DDD
    // /  VV
    //   ---
    //     2
    //
    // 12 are the bits in the result, and DDD goes to the remainder
    //
    // Proofs:
    // 8/2 -> Overflow (10xx >= 10)
    // 4/2 -> Ok (01xx < 10) -> 010x / 10 = '1' -> 010-10=0 -> 10 > 00 so early out, remaining bits 0 -> 10, remainder 0, 1 clock
    // 1/1 -> Ok (00xx < 01) -> 000x / 01 = '0' -> x001 / 01 = '1' -> 001-01=0 -> 01>00 so finished (either way) -> 01, remainder 1, 2 clocks
    // 5/2 -> Ok (01xx < 10) -> 010 / 10 = '1' -> 010-10=0 -> 10 > 001 so done -> 10, remainder 1, 1 clock
    // 
    // TODO: We should be able to prove on real hardware that the early out works like this (greater than, and not 0) with a few choice test cases

    Word x1,x2; 
    uint32_t x3;
    
    // minimum possible when division does not occur
    AddCycleCount(16);

    FormatIXraw;
    x2=ROMWORD(S);  // read source MSW

    D=WP+(D<<1);
    x3=ROMWORD(D);  // read dest
    
    // E/A: When the source operand is greater than the first word of the destination
    // operand, normal division occurs. If the source operand is less than or equal to
    // the first word of the destination operand, normal division results in a quotient
    // that cannot be represented in a 16-bit word. In this case, the computer sets the
    // overflow status bit, leaves the destination operand unchanged, and cancels the
    // division operation.
    if (x2>x3)                      // x2 can not be zero because they're unsigned                                      
    { 
        x3=(x3<<16)|ROMWORD(D+2);   // read source LSW
#if 0
        x1=(x3/x2)&0xffff;
        WRWORD(D,x1);
        x1=(x3%x2)&0xffff;
        WRWORD(D+2,x1);
#else
        // lets try it the iterative way, should be able to afford it
        // tested with 10,000,000 random combinations, should be accurate :)
        uint32_t mask = (0xFFFF8000);   // 1 extra bit, as noted above
        uint32_t divisor = (x2<<15);    // slide up into place
        int cnt = 16;                   // need to fill 16 bits
        x1 = 0;                         // initialize quotient, remainder will end up in x3 LSW
        while (x2 <= x3) {
            x1<<=1;
            x1 &= 0xffff;
            if ((x3&mask) >= divisor) {
                x1|=1;
                x3-=divisor;
            }
            mask>>=1;
            divisor>>=1;
            --cnt;
            AddCycleCount(1);
        }
        if (cnt < 0) {
            debug_write("Warning: Division bug. Send to Tursi if you can.");
        }
        while (cnt-- > 0) {
            // handle the early-out case
            x1<<=1;
        }
        WRWORD(D,x1);               // write dest quotient
        WRWORD(D+2,x3&0xffff);      // write dest remainder
#endif
        reset_OV;
        AddCycleCount(92-16);       // base ticks
    }
    else
    {
        set_OV;                     // division wasn't possible - change nothing
    }
}

void TMS9900::op_inc()
{ 
    // INCrement: INC src
    
    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=x1+1);

    reset_EQ_LGT_AGT_C_OV;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ_OV_C;
}

void TMS9900::op_inct()
{ 
    // INCrement by Two: INCT src
    
    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=x1+2);
    
    reset_EQ_LGT_AGT_C_OV;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;

    if (x3<2) set_C;
    if ((x3==0x8000)||(x3==0x8001)) set_OV;
}

void TMS9900::op_mpy()
{ 
    // MultiPlY: MPY src, dst
    // Multiply src by dest and store 32-bit result

    // Base cycles: 52
    // 5 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest MSW
    //  Write dest LSW

    Word x1;
    uint32_t x3;
    
    AddCycleCount(52);

    FormatIXraw;
    x1=ROMWORD(S);      // read source
    
    D=WP+(D<<1);
    x3=ROMWORD(D);      // read dest
    x3=x3*x1;
    WRWORD(D,(x3>>16)&0xffff);  // write dest MSW
    WRWORD(D+2,(x3&0xffff));    // write dest LSW
}

void TMS9900::op_neg()
{ 
    // NEGate: NEG src

    // TODO: can we measure this? it doesn't
    // seem to make sense that INC/DEC would be 10
    // cycles but this one is 12...

    // Base cycles: 12
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=(~x1)+1);
    AddCycleCount(2);       // this one is a little slower

    reset_EQ_LGT_AGT_C_OV;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ_OV_C;
}

void TMS9900::op_s()
{ 
    // Subtract: S src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatI(x3=x2-x1);	// Read Source, Read Dest
    WRWORD(D,x3);       // Write Dest

    reset_EQ_LGT_AGT_C_OV;
    ST |= WStatusLookup[x3&0xffff]&mask_LGT_AGT_EQ;

    // any number minus 0 sets carry.. my theory is that converting 0 to the two's complement
    // is causing the carry flag to be set.
    if ((x3<x2) || (x1==0)) set_C;
    if (((x1&0x8000)!=(x2&0x8000))&&((x3&0x8000)!=(x2&0x8000))) set_OV;
}

void TMS9900::op_sb()
{ 
    // Subtract Byte: SB src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatIb(x3=x2-x1);	// Read Source, Read Dest
    WRWORD(D,xD);       // write dest

    reset_EQ_LGT_AGT_C_OV_OP;
    ST|=BStatusLookup[x3]&mask_LGT_AGT_EQ_OP;

    // any number minus 0 sets carry.. my theory is that converting 0 to the two's complement
    // is causing the carry flag to be set.
    if ((x3<x2) || (x1==0)) set_C;
    if (((x1&0x80)!=(x2&0x80))&&((x3&0x80)!=(x2&0x80))) set_OV;
}

void TMS9900::op_b()
{ 
    // Branch: B src
    // Unconditional absolute branch

    // Base cycles: 8
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read source (confirmed by system design book)

    AddCycleCount(8);

    FormatVIraw;
    (void)x1;
    SetPC(S);
}

void TMS9900::op_bl()
{   
    // Branch and Link: BL src
    // Essentially a subroutine jump - return address is stored in R11
    // Note there is no stack, and no official return function.
    // A return is simply B *R11. Some assemblers define RT as this.

    // Base cycles: 12
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write return

    AddCycleCount(12);

    FormatVIraw;        // read source (unused)
    (void)x1;
    if (0 == GetReturnAddress()) {
        SetReturnAddress(PC);
    }
    WRWORD(WP+22,PC);   // write return
    SetPC(S);
}

void TMS9900::op_blwp()
{ 
    // Branch and Load Workspace Pointer: BLWP src
    // A context switch. The src address points to a 2 word table.
    // the first word is the new workspace address, the second is
    // the address to branch to. The current Workspace Pointer,
    // Program Counter (return address), and Status register are
    // stored in the new R13, R14 and R15, respectively
    // Return is performed with RTWP

    // Base cycles: 26
    // 6 memory accesses:
    //  Read instruction (already done)
    //  Read WP
    //  Write WP->R13
    //  Write PC->R14
    //  Write ST->R15
    //  Read PC

    // Note that there is no "read source" (BLWP R0 /does/ branch with >8300, it doesn't fetch R0)
    // TODO: We need to time out this instruction and verify that analysis.

    Word x2;
    
    AddCycleCount(26);

    FormatVIraw;            // read source (WP)
    if (0 == GetReturnAddress()) {
        SetReturnAddress(PC);
    }
    x2=WP;
    SetWP(x1);
    WRWORD(WP+26,x2);       // write WP->R13
    WRWORD(WP+28,PC);       // write PC->R14
    WRWORD(WP+30,ST);       // write ST->R15
    SetPC(ROMWORD(S+2));    // read PC

    // TODO: is it possible to conceive a test where the BLWP vectors being written affects
    // where it jumps to? That is - can we prove the above order of operation is correct
    // by placing the workspace and the vector such that they overlap, and then seeing
    // where it actually jumps to on hardware?

    SET_SKIP_INTERRUPT;
}

void TMS9900::op_jeq()
{ 
    // Jump if equal: JEQ dsp
    // Conditional relative branch. The displacement is a signed byte representing
    // the number of words to branch

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(ST_EQ);
}

void TMS9900::op_jgt()
{ 
    // Jump if Greater Than: JGT dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(ST_AGT);
}

void TMS9900::op_jhe()
{ 
    // Jump if High or Equal: JHE dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj((ST_LGT)||(ST_EQ));
}

void TMS9900::op_jh()
{ 
    // Jump if High: JH dsp
    
    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj((ST_LGT)&&(!ST_EQ));
}

void TMS9900::op_jl()
{
    // Jump if Low: JL dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj((!ST_LGT)&&(!ST_EQ));
}

void TMS9900::op_jle()
{ 
    // Jump if Low or Equal: JLE dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj((!ST_LGT)||(ST_EQ));
}

void TMS9900::op_jlt()
{ 
    // Jump if Less Than: JLT dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj((!ST_AGT)&&(!ST_EQ));
}

void TMS9900::op_jmp()
{ 
    // JuMP: JMP dsp
    // (unconditional)
    
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(true);
}

void TMS9900::op_jnc()
{ 
    // Jump if No Carry: JNC dsp
    
    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(!ST_C);
}

void TMS9900::op_jne()
{ 
    // Jump if Not Equal: JNE dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(!ST_EQ);
}

void TMS9900::op_jno()
{ 
    // Jump if No Overflow: JNO dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(!ST_OV);
}

void TMS9900::op_jop()
{ 
    // Jump on Odd Parity: JOP dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(ST_OP);
}

void TMS9900::op_joc()
{ 
    // Jump On Carry: JOC dsp

    // Jump taken:
    // Base cycles: 10
    // 1 memory access:
    //  Read instruction (already done)

    // Jump not taken:
    // Base cycles: 8
    // 1 memory access:
    //  Read instruction (already done)
    FormatIIj(ST_C);
}

void TMS9900::op_rtwp()
{ 
    // ReTurn with Workspace Pointer: RTWP
    // The matching return for BLWP, see BLWP for description

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read ST<-R15
    //  Read WP<-R13
    //  Read PC<-R14

    AddCycleCount(14);

    FormatVII;

    SetST(ROMWORD(WP+30));      // ST<-R15
    SetPC(ROMWORD(WP+28));      // PC<-R14 (order matter?)
    SetWP(ROMWORD(WP+26));      // WP<-R13 -- needs to be last!
}

void TMS9900::op_x()
{ 
    // eXecute: X src
    // The argument is interpreted as an instruction and executed

    // Base cycles: 8 - added to the execution time of the instruction minus 4 clocks and 1 memory
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read source

    if (X_flag!=0) 
    {
        debug_write("Recursive X instruction!!!!!");
        // While it will probably work (recursive X), I don't like the idea ;)
        // Barry Boone says that it does work, although if you recursively
        // call X in a register (ie: X R4 that contains X R4), you will lock
        // up the CPU so bad even the LOAD interrupt can't recover it.
        // We don't emulate that lockup here in Classic99, but of course it
        // will just spin forever.
        // TODO: we should try this ;)
    }
    AddCycleCount(8-4); // For X, add this time to the execution time of the instruction found at the source address, minus 4 clock cycles and 1 memory access. 
                        // we already accounted for the memory access (the instruction is going to be already in S)

    FormatVIraw;        // read source
    in=x1;              // opcode needs to be in 'in'

    X_flag=PC;          // set flag and save true post-X address for the JMPs (AFTER X's oprands but BEFORE the instruction's oprands, if any)

    CALL_MEMBER_FN(this, opcode[in])();

    X_flag=0;           // clear flag
}

void TMS9900::op_xop()
{ 
    // eXtended OPeration: XOP src ???
    // The CPU maintains a jump table starting at 0x0040, containing BLWP style
    // jumps for each operation. In addition, the new R11 gets a copy of the address of
    // the source operand.
    // Apparently not all consoles supported both XOP 1 and 2 (depends on the ROM)
    // so it is probably rarely, if ever, used on the TI99.
    
    // Base cycles: 36
    // 8 memory accesses:
    //  Read instruction (already done)
    //  Read source (unused)
    //  Read WP
    //  Write Src->R11
    //  Write WP->R13
    //  Write PC->R14
    //  Write ST->R15
    //  Read PC

    Word x1;

    AddCycleCount(36);

    FormatIXraw;
    D&=0xf;

    ROMWORD(S);         // read source (unused)

    x1=WP;
    SetWP(ROMWORD(0x0040+(D<<2)));  // read WP
    WRWORD(WP+22,S);                // write Src->R11
    WRWORD(WP+26,x1);               // write WP->R13
    WRWORD(WP+28,PC);               // write PC->R14
    WRWORD(WP+30,ST);               // write ST->R15
    SetPC(ROMWORD(0x0042+(D<<2)));  // read PC
    set_XOP;

    SET_SKIP_INTERRUPT;
}

void TMS9900::op_c()
{ 
    // Compare words: C src, dst
    
    // Base cycles: 14
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    FormatI(x3=0);	// Read Source, Read Dest
    (void)x3;

    reset_LGT_AGT_EQ;
    if (x1>x2) set_LGT;
    if (x1==x2) set_EQ;
    if ((x1&0x8000)==(x2&0x8000)) {
        if (x1>x2) set_AGT;
    } else {
        if (x2&0x8000) set_AGT;
    }
}

void TMS9900::op_cb()
{ 
    // Compare Bytes: CB src, dst

    // Base cycles: 14
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    FormatIb(x3=0);	// Read Source, Read Dest
    (void)x3;

    reset_LGT_AGT_EQ_OP;
    if (x1>x2) set_LGT;
    if (x1==x2) set_EQ;
    if ((x1&0x80)==(x2&0x80)) {
        if (x1>x2) set_AGT;
    } else {
        if (x2&0x80) set_AGT;
    }
    ST|=BStatusLookup[x3]&BIT_OP;
}

void TMS9900::op_ci()
{ 
    // Compare Immediate: CI src, imm
    
    // Base cycles: 14
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest

    Word x3;

    AddCycleCount(14);

    FormatVIII_1raw;    // read source
    x3=ROMWORD(D);      // read dest
  
    reset_LGT_AGT_EQ;
    if (x3>S) set_LGT;
    if (x3==S) set_EQ;
    if ((x3&0x8000)==(S&0x8000)) {
        if (x3>S) set_AGT;
    } else {
        if (S&0x8000) set_AGT;
    }
}

void TMS9900::op_coc()
{ 
    // Compare Ones Corresponding: COC src, dst
    // Basically comparing against a mask, if all set bits in the src match
    // set bits in the dest (mask), the equal bit is set

    // Base cycles: 14
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    FormatIII(x3=x1&x2);

    if (x3==x1) set_EQ; else reset_EQ;
}

void TMS9900::op_czc()
{ 
    // Compare Zeros Corresponding: CZC src, dst
    // The opposite of COC. Each set bit in the dst (mask) must
    // match up with a zero bit in the src to set the equals flag

    // Base cycles: 14
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    FormatIII(x3=x1&x2);

    if (x3==0) set_EQ; else reset_EQ;
}

void TMS9900::op_ldcr()
{ 
    // LoaD CRu - LDCR src, dst
    // Writes dst bits serially out to the CRU registers
    // The CRU is the 9901 Communication chip, tightly tied into the 9900.
    // It's serially accessed and has 4096 single bit IO registers.
    // It thinks 0 is true and 1 is false.
    // All addresses are offsets from the value in R12, which is divided by 2

    // Base cycles: 20 + 2 per count (count of 0 represents 16)
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source (byte access if count = 1-8)
    //  Read R12

    Word x1,x3,cruBase;
    
    AddCycleCount(20);  // base count

    FormatIV;
    if (D==0) D=16;     // this also makes the timing of (0=52 cycles) work - 2*16=32+20=52
    x1=(D<9 ? RCPUBYTE(S) : ROMWORD(S));    // read source
  
    x3=1;

    // CRU base address - R12 bits 3-14 (0=MSb)
    // 0001 1111 1111 1110
    cruBase=(ROMWORD(WP+24)>>1)&0xfff;      // read R12
    for (int x2=0; x2<D; x2++)
    { 
        theCore->writeIOByte(cruBase+x2, nCycleCount, ACCESS_WRITE, (x1&x3) ? 1 : 0);
        x3=x3<<1;
    }

    AddCycleCount(2*D);

    // TODO: the data manual says this return is not true - test
    // whether a word load affects the other status bits
    //  if (D>8) return;

    reset_LGT_AGT_EQ;
    if (D<9) {
        reset_OP;
        ST|=BStatusLookup[x1&0xff]&mask_LGT_AGT_EQ_OP;
    } else {
        ST|=WStatusLookup[x1]&mask_LGT_AGT_EQ;
    }
}

void TMS9900::op_sbo()
{ 
    // Set Bit On: SBO src
    // Sets a bit in the CRU
    
    // Base cycles: 12
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read R12
    FormatII;
    theCore->writeIOByte(add, nCycleCount, ACCESS_WRITE, 1);
}

void TMS9900::op_sbz()
{ 
    // Set Bit Zero: SBZ src
    // Zeros a bit in the CRU

    // Base cycles: 12
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read R12
    FormatII;
    theCore->writeIOByte(add, nCycleCount, ACCESS_WRITE, 0);
}

void TMS9900::op_stcr()
{ 
    // STore CRU: STCR src, dst
    // Stores dst bits from the CRU into src

    // Base cycles: C=0:60, C=1-7:42, C=8:44, C=9-15:58
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read R12
    //  Read source
    //  Write source (byte access if count = 1-8)

    Word x1,x3,x4, cruBase; 

    AddCycleCount(42);  // base value

    FormatIV;
    if (D==0) D=16;
    x1=0; x3=1;
  
    cruBase=(ROMWORD(WP+24)>>1)&0xfff;  // read R12
    for (int x2=0; x2<D; x2++)
    { 
        x4 = theCore->readIOByte(cruBase+x2, nCycleCount, ACCESS_READ);
        if (x4) 
        {
            x1=x1|x3;
        }
        x3<<=1;
    }

    if (D<9) 
    {
        WCPUBYTE(S,(Byte)(x1&0xff));  // Read source, write source
    }
    else 
    {
        ROMWORD(S, ACCESS_RMW);       // read source (wasted)
        WRWORD(S,x1);                 // write source
    }

    if (D<8) {
//      AddCycleCount(42-42);         // 0 more cycles, so just commented out so the compiler doesn't worry about it
    } else if (D < 9) {
        AddCycleCount(44-42);
    } else if (D < 16) {
        AddCycleCount(58-42);
    } else {
        AddCycleCount(60-42);
    }

    // TODO: the data manual says this return is not true - test
    // whether a word load affects the other status bits
    //if (D>8) return;

    reset_LGT_AGT_EQ;
    if (D<9) {
        reset_OP;
        ST|=BStatusLookup[x1&0xff]&mask_LGT_AGT_EQ_OP;
    } else {
        ST|=WStatusLookup[x1]&mask_LGT_AGT_EQ;
    }
}

void TMS9900::op_tb()
{ 
    // Test Bit: TB src
    // Tests a CRU bit

    // Base cycles: 12
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read R12
    FormatII;
    if (theCore->readIOByte(add, nCycleCount, ACCESS_READ)) set_EQ; else reset_EQ;
}

// These instructions are valid 9900 instructions but are invalid on the TI-99, as they generate
// improperly decoded CRU instructions.

void TMS9900::op_ckof()
{ 
    // Base cycles: 12
    // 1 memory accesses:
    //  Read instruction (already done)

    AddCycleCount(12);

    FormatVII;
    debug_write("ClocK OFf instruction encountered!");                 // not supported on 99/4A
    // This will set A0-A2 to 110 and pulse CRUCLK (so not emulated)
}

void TMS9900::op_ckon()
{ 
    // Base cycles: 12
    // 1 memory accesses:
    //  Read instruction (already done)

    AddCycleCount(12);

    FormatVII;
    debug_write("ClocK ON instruction encountered!");                  // not supported on 99/4A
    // This will set A0-A2 to 101 and pulse CRUCLK (so not emulated)
}

void TMS9900::op_idle()
{
    // Base cycles: 12
    // 1 memory accesses:
    //  Read instruction (already done)
    AddCycleCount(12);

    FormatVII;
    debug_write("IDLE instruction encountered!");                      // not supported on 99/4A
    // This sets A0-A2 to 010, and pulses CRUCLK until an interrupt is received
    // Although it's not supposed to be used on the TI, at least one game
    // (Slymoids) uses it - perhaps to sync with the VDP? So we'll emulate it someday

    StartIdle();
}

void TMS9900::op_rset()
{
    // Base cycles: 12
    // 1 memory accesses:
    //  Read instruction (already done)

    AddCycleCount(12);

    FormatVII;
    debug_write("ReSET instruction encountered!");                     // not supported on 99/4A
    // This will set A0-A2 to 011 and pulse CRUCLK (so not emulated)
    // However, it does have an effect, it zeros the interrupt mask
    ST&=0xfff0;
}

void TMS9900::op_lrex()
{
    // Base cycles: 12
    // 1 memory accesses:
    //  Read instruction (already done)

    AddCycleCount(12);

    FormatVII;
    debug_write("Load or REstart eXecution instruction encountered!"); // not supported on 99/4A
    // This will set A0-A2 to 111 and pulse CRUCLK (so not emulated)
}

void TMS9900::op_li()
{
    // Base cycles: 12
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read immediate
    //  Write register

    // note no read-before write!

    // Load Immediate: LI src, imm

    AddCycleCount(12);

    FormatVIII_1raw;    // read immediate
    WRWORD(D,S);        // write register
    
    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[S]&mask_LGT_AGT_EQ;
}

void TMS9900::op_limi()
{ 
    // Load Interrupt Mask Immediate: LIMI imm
    // Sets the CPU interrupt mask

    // Base cycles: 16
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read immediate

    AddCycleCount(16);

    FormatVIII_1raw;            // read immediate
    SetST((ST & 0xfff0) | (S & 0x0f));
}

void TMS9900::op_lwpi()
{ 
    // Load Workspace Pointer Immediate: LWPI imm
    // changes the Workspace Pointer

    // Base cycles: 10
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Read immediate

    AddCycleCount(10);

    FormatVIII_1raw;       // read immediate
    SetWP(S);
}

void TMS9900::op_mov()
{ 
    // MOVe words: MOV src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatI(x3=x1);	// Read Source, Read Dest
    (void)x2;
    WRWORD(D,x3);   // Write Dest
  
    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;
}

void TMS9900::op_movb()
{ 
    // MOVe Bytes: MOVB src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatIb(x3=x1);	// Read Source, Read Dest
    (void)x3;
    (void)x2;
    WRWORD(D,xD);       // write dest

    reset_LGT_AGT_EQ_OP;
    ST|=BStatusLookup[x1]&mask_LGT_AGT_EQ_OP;
}

void TMS9900::op_stst()
{ 
    // STore STatus: STST src
    // Copy the status register to memory

    // Base cycles: 8
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Write dest
    FormatVIII_0(ST);
}

void TMS9900::op_stwp()
{ 
    // STore Workspace Pointer: STWP src
    // Copy the workspace pointer to memory

    // Base cycles: 8
    // 2 memory accesses:
    //  Read instruction (already done)
    //  Write dest
    FormatVIII_0(WP);
}

void TMS9900::op_swpb()
{ 
    // SWaP Bytes: SWPB src
    // swap the high and low bytes of a word

    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=((x1&0xff)<<8)|(x1>>8));
}

void TMS9900::op_andi()
{ 
    // AND Immediate: ANDI src, imm

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read immediate
    //  Read dest
    //  Write dest
    FormatVIII_1(x3=x1&x2);
  
    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[x2]&mask_LGT_AGT_EQ;
}

void TMS9900::op_ori()
{ 
    // OR Immediate: ORI src, imm

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read immediate
    //  Read dest
    //  Write dest
    FormatVIII_1(x3=x1|x2);

    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[x2]&mask_LGT_AGT_EQ;
}

void TMS9900::op_xor()
{ 
    // eXclusive OR: XOR src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatIII(x3=x1^x2);
    WRWORD(D,x3);   // write dest
  
    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;
}

void TMS9900::op_inv()
{ 
    // INVert: INV src

    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=~x1);

    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;
}

void TMS9900::op_clr()
{ 
    // CLeaR: CLR src
    // sets word to 0

    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=0);
}

void TMS9900::op_seto()
{ 
    // SET to One: SETO src
    // sets word to 0xffff

    // Base cycles: 10
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source
    FormatVI(x3=0xffff);
}

void TMS9900::op_soc()
{ 
    // Set Ones Corresponding: SOC src, dst
    // Essentially performs an OR - setting all the bits in dst that
    // are set in src

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatI(x3=x1|x2)	// Read Source, Read Dest
    WRWORD(D,x3);       // write dest

    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;
}

void TMS9900::op_socb()
{ 
    // Set Ones Corresponding, Byte: SOCB src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatIb(x3=x1|x2);	// Read Source, Read Dest
    WRWORD(D,xD);       // write dest

    reset_LGT_AGT_EQ_OP;
    ST|=BStatusLookup[x3]&mask_LGT_AGT_EQ_OP;
}

void TMS9900::op_szc()
{ 
    // Set Zeros Corresponding: SZC src, dst
    // Zero all bits in dest that are zeroed in src

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatI(x3=(~x1)&x2)	// Read Source, Read Dest
    WRWORD(D,x3);           // write dest
  
    reset_LGT_AGT_EQ;
    ST|=WStatusLookup[x3]&mask_LGT_AGT_EQ;
}

void TMS9900::op_szcb()
{ 
    // Set Zeros Corresponding, Byte: SZCB src, dst

    // Base cycles: 14
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Read dest
    //  Write dest
    FormatIb(x3=(~x1)&x2);	// Read Source, Read Dest
    WRWORD(D,xD);       // write dest

    reset_LGT_AGT_EQ_OP;
    ST|=BStatusLookup[x3]&mask_LGT_AGT_EQ_OP;
}

void TMS9900::op_sra()
{ 
    // Shift Right Arithmetic: SRA src, dst
    // For the shift instructions, a count of '0' means use the
    // value in register 0. If THAT is zero, the count is 16.
    // The arithmetic operations preserve the sign bit

    // Count != 0
    // Base cycles: 12 + 2*count
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source

    // Count = 0
    // Base cycles: 20 + 2*count (in R0 least significant nibble, 0=16)
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read R0
    //  Read source
    //  Write source
    Word x2,x3,x4; 

    FormatV;

    x4=x1&0x8000;
    x3=0;
  
    for (x2=0; x2<D; x2++)
    { 
        x3=x1&1;   /* save carry */
        x1=x1>>1;  /* shift once */
        x1=x1|x4;  /* extend sign bit */
    }
    WRWORD(S,x1);               // write source
  
    reset_EQ_LGT_AGT_C;
    ST|=WStatusLookup[x1]&mask_LGT_AGT_EQ;

    if (x3) set_C;
}

void TMS9900::op_srl()
{ 
    // Shift Right Logical: SRL src, dst
    // The logical shifts do not preserve the sign

    // Count != 0
    // Base cycles: 12 + 2*count
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source

    // Count = 0
    // Base cycles: 20 + 2*count (in R0 least significant nibble, 0=16)
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read R0
    //  Read source
    //  Write source
    Word x2,x3;

    FormatV;

    x3=0;
    for (x2=0; x2<D; x2++)
    { 
        x3=x1&1;
        x1=x1>>1;
    }
    WRWORD(S,x1);           // write source

    reset_EQ_LGT_AGT_C;
    ST|=WStatusLookup[x1]&mask_LGT_AGT_EQ;

    if (x3) set_C;
}

void TMS9900::op_sla()
{ 
    // Shift Left Arithmetic: SLA src, dst

    // Count != 0
    // Base cycles: 12 + 2*count
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source

    // Count = 0
    // Base cycles: 20 + 2*count (in R0 least significant nibble, 0=16)
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read R0
    //  Read source
    //  Write source
    Word x2,x3,x4;

    FormatV;

    x4=x1&0x8000;
    reset_EQ_LGT_AGT_C_OV;

    x3=0;
    for (x2=0; x2<D; x2++)
    { 
        x3=x1&0x8000;
        x1=x1<<1;
        if ((x1&0x8000)!=x4) set_OV;
    }
    WRWORD(S,x1);           // write source
  
    ST|=WStatusLookup[x1]&mask_LGT_AGT_EQ;

    if (x3) set_C;
}

void TMS9900::op_src()
{ 
    // Shift Right Circular: SRC src, dst
    // Circular shifts pop bits off one end and onto the other
    // The carry bit is not a part of these shifts, but it set
    // as appropriate

    // Count != 0
    // Base cycles: 12 + 2*count
    // 3 memory accesses:
    //  Read instruction (already done)
    //  Read source
    //  Write source

    // Count = 0
    // Base cycles: 20 + 2*count (in R0 least significant nibble, 0=16)
    // 4 memory accesses:
    //  Read instruction (already done)
    //  Read R0
    //  Read source
    //  Write source
    Word x2,x4;

    x4 = 0;
    FormatV;

    for (x2=0; x2<D; x2++)
    { 
        x4=x1&0x1;
        x1=x1>>1;
        if (x4) 
        {
            x1=x1|0x8000;
        }
    }
    WRWORD(S,x1);           // write source
  
    reset_EQ_LGT_AGT_C;
    ST|=WStatusLookup[x1]&mask_LGT_AGT_EQ;

    if (x4) set_C;

    AddCycleCount(2*D);
}

void TMS9900::op_bad()
{ 
    char buf[128];

    // Base cycles: 6
    // 1 memory accesses:
    //  Read instruction (already done)
    AddCycleCount(6);

    FormatVII;
    sprintf(buf, "Illegal opcode (%04X)", in);
    debug_write(buf);       // Don't know this Opcode
    if (getInterestingData(DATA_BREAK_ON_ILLEGAL) == DATA_TRUE) theCore->triggerBreakpoint();
}

#ifdef BUILD_CPU
////////////////////////////////////////////////////////////////////////
// Fill the CPU Opcode Address table
////////////////////////////////////////////////////////////////////////
void TMS9900::buildcpu() {
    for (int in=0; in<65536; ++in)
    { 
        int x=(in&0xf000)>>12;
        switch(x) { 
            case 0: opcode0(in);                    break;
            case 1: opcode1(in);                    break;
            case 2: opcode2(in);                    break;
            case 3: opcode3(in);                    break;
            case 4: opcode[in]=&TMS9900::op_szc;    break;
            case 5: opcode[in]=&TMS9900::op_szcb;   break;
            case 6: opcode[in]=&TMS9900::op_s;      break;
            case 7: opcode[in]=&TMS9900::op_sb;     break;
            case 8: opcode[in]=&TMS9900::op_c;      break;
            case 9: opcode[in]=&TMS9900::op_cb;     break;
            case 10:opcode[in]=&TMS9900::op_a;      break;
            case 11:opcode[in]=&TMS9900::op_ab;     break;
            case 12:opcode[in]=&TMS9900::op_mov;    break;
            case 13:opcode[in]=&TMS9900::op_movb;   break;
            case 14:opcode[in]=&TMS9900::op_soc;    break;
            case 15:opcode[in]=&TMS9900::op_socb;   break;
            default: opcode[in]=&TMS9900::op_bad;
        }
    } 

    // build the Word status lookup table
    for (int i=0; i<65536; i++) {
        WStatusLookup[i]=0;
        // LGT
        if (i>0) WStatusLookup[i]|=BIT_LGT;
        // AGT
        if ((i>0)&&(i<0x8000)) WStatusLookup[i]|=BIT_AGT;
        // EQ
        if (i==0) WStatusLookup[i]|=BIT_EQ;
        // C
        if (i==0) WStatusLookup[i]|=BIT_C;
        // OV
        if (i==0x8000) WStatusLookup[i]|=BIT_OV;
    }
    // And byte
    for (int i=0; i<256; i++) {
        Byte x=(Byte)(i&0xff);
        BStatusLookup[i]=0;
        // LGT
        if (i>0) BStatusLookup[i]|=BIT_LGT;
        // AGT
        if ((i>0)&&(i<0x80)) BStatusLookup[i]|=BIT_AGT;
        // EQ
        if (i==0) BStatusLookup[i]|=BIT_EQ;
        // C
        if (i==0) BStatusLookup[i]|=BIT_C;
        // OV
        if (i==0x80) BStatusLookup[i]|=BIT_OV;
        // OP
        int z;
        for (z=0; x; x&=(x-1)) z++;                     // black magic!
        if (z&1) BStatusLookup[i]|=BIT_OP;              // set bit if an odd number
    }
}

///////////////////////////////////////////////////////////////////////////
// CPU Opcode 0 helper function
///////////////////////////////////////////////////////////////////////////
void TMS9900::opcode0(int in) {
    int x=(in&0x0f00)>>8;

    switch(x) { 
        case 2: opcode02(in);                   break;
        case 3: opcode03(in);                   break;
        case 4: opcode04(in);                   break;
        case 5: opcode05(in);                   break;
        case 6: opcode06(in);                   break;
        case 7: opcode07(in);                   break;
        case 8: opcode[in]=&TMS9900::op_sra;    break;
        case 9: opcode[in]=&TMS9900::op_srl;    break;
        case 10:opcode[in]=&TMS9900::op_sla;    break;
        case 11:opcode[in]=&TMS9900::op_src;    break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

////////////////////////////////////////////////////////////////////////////
// CPU Opcode 02 helper function
////////////////////////////////////////////////////////////////////////////
void TMS9900::opcode02(int in) { 
    int x=(in&0x00e0)>>4;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_li;     break;
        case 2: opcode[in]=&TMS9900::op_ai;     break;
        case 4: opcode[in]=&TMS9900::op_andi;   break;
        case 6: opcode[in]=&TMS9900::op_ori;    break;
        case 8: opcode[in]=&TMS9900::op_ci;     break;
        case 10:opcode[in]=&TMS9900::op_stwp;   break;
        case 12:opcode[in]=&TMS9900::op_stst;   break;
        case 14:opcode[in]=&TMS9900::op_lwpi;   break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

////////////////////////////////////////////////////////////////////////////
// CPU Opcode 03 helper function
////////////////////////////////////////////////////////////////////////////
void TMS9900::opcode03(int in) { 
    int x=(in&0x00e0)>>4;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_limi; break;
        case 4: opcode[in]=&TMS9900::op_idle; break;
        case 6: opcode[in]=&TMS9900::op_rset; break;
        case 8: opcode[in]=&TMS9900::op_rtwp; break;
        case 10:opcode[in]=&TMS9900::op_ckon; break;
        case 12:opcode[in]=&TMS9900::op_ckof; break;
        case 14:opcode[in]=&TMS9900::op_lrex; break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

///////////////////////////////////////////////////////////////////////////
// CPU Opcode 04 helper function
///////////////////////////////////////////////////////////////////////////
void TMS9900::opcode04(int in) { 
    int x=(in&0x00c0)>>4;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_blwp;   break;
        case 4: opcode[in]=&TMS9900::op_b;      break;
        case 8: opcode[in]=&TMS9900::op_x;      break;
        case 12:opcode[in]=&TMS9900::op_clr;    break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

//////////////////////////////////////////////////////////////////////////
// CPU Opcode 05 helper function
//////////////////////////////////////////////////////////////////////////
void TMS9900::opcode05(int in)
{ 
    int x=(in&0x00c0)>>4;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_neg;    break;
        case 4: opcode[in]=&TMS9900::op_inv;    break;
        case 8: opcode[in]=&TMS9900::op_inc;    break;
        case 12:opcode[in]=&TMS9900::op_inct;   break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

////////////////////////////////////////////////////////////////////////
// CPU Opcode 06 helper function
////////////////////////////////////////////////////////////////////////
void TMS9900::opcode06(int in) { 
    int x=(in&0x00c0)>>4;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_dec;    break;
        case 4: opcode[in]=&TMS9900::op_dect;   break;
        case 8: opcode[in]=&TMS9900::op_bl;     break;
        case 12:opcode[in]=&TMS9900::op_swpb;   break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

////////////////////////////////////////////////////////////////////////
// CPU Opcode 07 helper function
////////////////////////////////////////////////////////////////////////
void TMS9900::opcode07(int in) { 
    int x=(in&0x00c0)>>4;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_seto; break;
        case 4: opcode[in]=&TMS9900::op_abs;  break;
        default: opcode[in]=&TMS9900::op_bad;
    }   
}

////////////////////////////////////////////////////////////////////////
// CPU Opcode 1 helper function
////////////////////////////////////////////////////////////////////////
void TMS9900::opcode1(int in) { 
    int x=(in&0x0f00)>>8;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_jmp;    break;
        case 1: opcode[in]=&TMS9900::op_jlt;    break;
        case 2: opcode[in]=&TMS9900::op_jle;    break;
        case 3: opcode[in]=&TMS9900::op_jeq;    break;
        case 4: opcode[in]=&TMS9900::op_jhe;    break;
        case 5: opcode[in]=&TMS9900::op_jgt;    break;
        case 6: opcode[in]=&TMS9900::op_jne;    break;
        case 7: opcode[in]=&TMS9900::op_jnc;    break;
        case 8: opcode[in]=&TMS9900::op_joc;    break;
        case 9: opcode[in]=&TMS9900::op_jno;    break;
        case 10:opcode[in]=&TMS9900::op_jl;     break;
        case 11:opcode[in]=&TMS9900::op_jh;     break;
        case 12:opcode[in]=&TMS9900::op_jop;    break;
        case 13:opcode[in]=&TMS9900::op_sbo;    break;
        case 14:opcode[in]=&TMS9900::op_sbz;    break;
        case 15:opcode[in]=&TMS9900::op_tb;     break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

////////////////////////////////////////////////////////////////////////
// CPU Opcode 2 helper function
////////////////////////////////////////////////////////////////////////
void TMS9900::opcode2(int in) { 
    int x=(in&0x0c00)>>8;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_coc; break;
        case 4: opcode[in]=&TMS9900::op_czc; break;
        case 8: opcode[in]=&TMS9900::op_xor; break;
        case 12:opcode[in]=&TMS9900::op_xop; break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

////////////////////////////////////////////////////////////////////////
// CPU Opcode 3 helper function
////////////////////////////////////////////////////////////////////////
void TMS9900::opcode3(int in) { 
    int x=(in&0x0c00)>>8;

    switch(x) { 
        case 0: opcode[in]=&TMS9900::op_ldcr; break;
        case 4: opcode[in]=&TMS9900::op_stcr; break;
        case 8: opcode[in]=&TMS9900::op_mpy;  break;
        case 12:opcode[in]=&TMS9900::op_div;  break;
        default: opcode[in]=&TMS9900::op_bad;
    }
}

#endif
