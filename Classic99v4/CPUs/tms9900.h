// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates a 3MHZ TMS9900, interconnected to an 8-bit bus
// for use with Classic99

#ifndef TMS9900_H
#define TMS9900_H

// ditching the 16 bit version to improve optimization
typedef uint8_t Byte;
typedef uint16_t Word;

// this enables the older buildcpu approach instead of the pre-defined table
//#define BUILD_CPU

// we set skip_interrupt to 2 because it's decremented after every instruction - including
// the one that sets it!
#define SET_SKIP_INTERRUPT skip_interrupt=2
// force the PC to always be 15-bit, can't store a 16-bit PC
#define ADDPC(x) { SetPC(PC+(x)); }

/////////////////////////////////////////////////////////////////////
// Status register defines
/////////////////////////////////////////////////////////////////////
#define BIT_LGT 0x8000
#define BIT_AGT 0x4000
#define BIT_EQ  0x2000
#define BIT_C   0x1000
#define BIT_OV  0x0800
#define BIT_OP  0x0400
#define BIT_XOP 0x0200

#define ST_LGT (ST & BIT_LGT)                       // Logical Greater Than
#define ST_AGT (ST & BIT_AGT)                       // Arithmetic Greater Than
#define ST_EQ  (ST & BIT_EQ)                        // Equal
#define ST_C   (ST & BIT_C)                         // Carry
#define ST_OV  (ST & BIT_OV)                        // Overflow
#define ST_OP  (ST & BIT_OP)                        // Odd Parity
#define ST_X   (ST & BIT_XOP)                       // Set during an XOP instruction
#define ST_INTMASK (ST&0x000f)                      // Interrupt mask (the TI uses only values 0 and 2)

#define set_LGT (ST|=0x8000)                        // Logical Greater than: >0x0000
#define set_AGT (ST|=0x4000)                        // Arithmetic Greater than: >0x0000 and <0x8000
#define set_EQ  (ST|=0x2000)                        // Equal: ==0x0000
#define set_C   (ST|=0x1000)                        // Carry: carry occurred during operation
#define set_OV  (ST|=0x0800)                        // Overflow: overflow occurred during operation
#define set_OP  (ST|=0x0400)                        // Odd parity: int has odd number of '1' bits
#define set_XOP (ST|=0x0200)                        // Executing 'XOP' function

#define reset_LGT (ST&=0x7fff)                      // Clear the flags
#define reset_AGT (ST&=0xbfff)
#define reset_EQ  (ST&=0xdfff)
#define reset_C   (ST&=0xefff)
#define reset_OV  (ST&=0xf7ff)
#define reset_OP  (ST&=0xfbff)
#define reset_XOP (ST&=0xfdff)

// Group clears
#define reset_EQ_LGT (ST&=0x5fff)
#define reset_LGT_AGT_EQ (ST&=0x1fff)
#define reset_LGT_AGT_EQ_OP (ST&=0x1bff)
#define reset_EQ_LGT_AGT_OV (ST&=0x17ff)
#define reset_EQ_LGT_AGT_C (ST&=0x0fff)
#define reset_EQ_LGT_AGT_C_OV (ST&=0x7ff)
#define reset_EQ_LGT_AGT_C_OV_OP (ST&=0x3ff)

// Assignment masks
#define mask_EQ_LGT (BIT_EQ|BIT_LGT)
#define mask_LGT_AGT_EQ (BIT_LGT|BIT_AGT|BIT_EQ)
#define mask_LGT_AGT_EQ_OP (BIT_LGT|BIT_AGT|BIT_EQ|BIT_OP)
#define mask_LGT_AGT_EQ_OV (BIT_LGT|BIT_AGT|BIT_EQ|BIT_OV)
#define mask_LGT_AGT_EQ_OV_C (BIT_LGT|BIT_AGT|BIT_EQ|BIT_OV|BIT_C)      // carry here used for INC and NEG only

// Like the FAQ says, be nice to people!
class TMS9900;
typedef void (TMS9900::*CPU9900Fctn)(void);			// now function pointers are just "CPU9900Fctn" type
#define CALL_MEMBER_FN(object, ptr) ((object)->*(ptr))

// Let's see what a CPU needs
class TMS9900 : public Classic99Peripheral {
public:
    TMS9900(Classic99System *core);
    virtual ~TMS9900();

    // ========== Classic99Peripheral interface ============

    // dummy read and write - IO flag is unused on most, but just in case they need to know
    // You can't read or write the CPU
    //virtual uint8_t read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) { (void)addr; (void)cycles; (void)rmw; return 0; }
    //virtual void write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) { (void)addr; (void)cycles; (void)rmw; (void)data; }

    // interface code
    // CPU doesn't have any dedicated I/O devices attached
    //virtual int hasAvailablePorts() { return 0; }    // number of pluggable ports (for serial, parallel, etc)
    //virtual int usesPlugPorts() { return 0; }   // number of ports that it has for attaching to (can't see this being more than 1)
    //virtual PORTTYPE getAvailablePort(int idx) { (void)idx; return NULL_PORT; }     // type of port exposed to connect to
    //virtual PORTTYPE getPlugPort(int idx) { (void)idx; return NULL_PORT; }          // type of port we need to plug into

    // send or receive a byte (or int) of data to a port, false on failure - what this means is device dependent
    //virtual bool sendPortData(int portIdx, int data) { return false; }       // this calls out to a plugged device
    //virtual bool receivePortData(int portIdx, int data) { return false; }    // this receives from a plugged device (normally only AFTER that device has called send)

    // file access for disks and cartridges
    //virtual int hasFileSystems() { return 0; }          // keep life simple, normally 0 or 1
    //virtual const char* getFileMask() { return ""; }    // return a file selector mask string, windows-style with NUL separators and double-NUL at the end
    //virtual bool loadFileData(unsigned char *buffer, int address, int length) { (void)buffer; (void)address; (void)length; return false; }  // passed from core, copy data if you need it cause it's destroyed after this call, return false if something is wrong with it

    // interface
    virtual bool init(int idx) override;                             // claim resources from the core system
    virtual bool operate(double timestamp) override;				 // process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
    virtual bool cleanup() override;                                 // release everything claimed in init, save NV data, etc

    // debug interface
    virtual void getDebugSize(int &x, int &y) override;             // dimensions of a text mode output screen - either being 0 means none
    virtual void getDebugWindow(char *buffer) override;             // output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
	//virtual void resetMemoryTracking() { }                        // reset memory tracking, if the peripheral has any

    // save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
    virtual int saveStateSize() override;                           // number of bytes needed to save state
    virtual bool saveState(unsigned char *buffer) override;         // write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
    virtual bool restoreState(unsigned char *buffer) override;      // restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize

    // addresses are from the system point of view
    // While the CPU does not get any memory or IO breakpoints,
    // it does breakpoint on PC, ST and WP settings. Or should. ;)
    // So does that mean implementing this interface?
    //virtual void testBreakpoint(bool isRead, int addr, bool isIO, int data);              // default class will do unless the device needs to be paging or such (calls system breakpoint function if set)

    // ========== TMS9900 functions ============
    virtual void reset();

	/////////////////////////////////////////////////////////////////////
	// Wrapper functions for memory access
	/////////////////////////////////////////////////////////////////////
	virtual Byte RCPUBYTE(Word src);
	virtual void WCPUBYTE(Word dest, Byte c);
	virtual int ROMWORD(Word src, MEMACCESSTYPE rmw=ACCESS_READ);
	virtual void WRWORD(Word dest, Word val);

	virtual	void TriggerInterrupt(int level);

	//////////////////////////////////////////////////////////////////////////
	// Get addresses for the destination and source arguments
	// Note: the format code letters are the official notation from Texas
	// instruments. See their TMS9900 documentation for details.
	// (Td, Ts, D, S, B, etc)
	// Note that some format codes set the destination type (Td) to
	// '4' in order to skip unneeded processing of the Destination address
	//////////////////////////////////////////////////////////////////////////
	void fixS();
	void fixD();

	// Helpers for what used to be global variables
	void StartIdle();
	void StopIdle();
	int  GetIdle();
	void SetReturnAddress(Word x);
	Word GetReturnAddress();
	void ResetCycleCount();
	void AddCycleCount(int val);
	int  GetCycleCount();
	// it's now important to always use these, not directly access the variables
	Word GetPC();
	void SetPC(Word x);
	Word GetST();
	void SetST(Word x);
	Word GetWP();
	void SetWP(Word x);

	////////////////////////////////////////////////////////////////////
	// Classic99 - 9900 CPU opcodes
	// Opcode functions follow
	// one function for each opcode (the mneumonic prefixed with "op_")
	// src - source address (register or memory - valid types vary)
	// dst - destination address
	// imm - immediate value
	// dsp - relative displacement
	////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// DO NOT USE wcpubyte or rcpubyte in here! You'll break the RMW
	// emulation and the cycle counting! The 9900 can only do int access.
	/////////////////////////////////////////////////////////////////////
	void op_a();
	void op_ab();
	void op_abs();
	void op_ai();
	void op_dec();
	void op_dect();
	void op_div();
	void op_inc();
	void op_inct();
	void op_mpy();
	void op_neg();
	void op_s();
	void op_sb();
	void op_b();
	void op_bl();
	void op_blwp();
	void op_jeq();
	void op_jgt();
	void op_jhe();
	void op_jh();
	void op_jl();
	void op_jle();
	void op_jlt();
	void op_jmp();
	void op_jnc();
	void op_jne();
	void op_jno();
	void op_jop();
	void op_joc();
	void op_rtwp();
	void op_x();
	void op_xop();
	void op_c();
	void op_cb();
	void op_ci();
	void op_coc();
	void op_czc();
	void op_ldcr();
	void op_sbo();
	void op_sbz();
	void op_stcr();
	void op_tb();

	// These instructions are valid 9900 instructions but are invalid on the TI-99, as they generate
	// improperly decoded CRU instructions.
	void op_ckof();
	void op_ckon();
	void op_idle();
	void op_rset();
	void op_lrex();

	// back to legal instructions
	void op_li();
	void op_limi();
	void op_lwpi();
	void op_mov();
	void op_movb();
	void op_stst();
	void op_stwp();
	void op_swpb();
	void op_andi();
	void op_ori();
	void op_xor();
	void op_inv();
	void op_clr();
	void op_seto();
	void op_soc();
	void op_socb();
	void op_szc();
	void op_szcb();
	void op_sra();
	void op_srl();
	void op_sla();
	void op_src();

	// not an opcode - illegal opcode handler
	void op_bad();

#ifdef BUILD_CPU
	////////////////////////////////////////////////////////////////////////
	// Fill the CPU Opcode Address table
	////////////////////////////////////////////////////////////////////////
	void buildcpu();
	void opcode0(int in);
	void opcode02(int in);
	void opcode03(int in);
	void opcode04(int in);
	void opcode05(int in);
	void opcode06(int in);
	void opcode07(int in);
	void opcode1(int in);
	void opcode2(int in);
	void opcode3(int in);
#endif

protected:
	// CPU variables
	Word PC;									    // Program Counter
	Word WP;										// Workspace Pointer
	Word X_flag;									// Set during an 'X' instruction, 0 if not active, else address of PC after the X (ignoring arguments if any)
	Word ST;										// Status register
	Word in,D,S,Td,Ts,B;							// Opcode interpretation
	volatile long nCycleCount;	                    // Cycles used for this operation

#ifdef BUILD_CPU
	CPU9900Fctn opcode[65536];						// CPU Opcode address table
#endif

	int idling;										// set when an IDLE occurs
	Word nReturnAddress;							// return address for step over
    int skip_interrupt;                             // interrupts are disabled for this many instructions

private:
#ifdef BUILD_CPU
    // Status register lookup tables
    Word WStatusLookup[64*1024];                     // word statuses
    Word BStatusLookup[256];                         // byte statuses
#endif

    const int32_t SAVE_STATE_VERSION = 1;
};


#endif
