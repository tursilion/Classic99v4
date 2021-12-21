// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates the GROMs attached to a TI-99/4 home computer

#ifndef CLASSIC99GROM_H
#define CLASSIC99GROM_H

#include "..\EmulatorSupport\peripheral.h"

// memory-mapped GROM memory emulation
// GROM is accessed via just a couple of access ports
// repeated in memory. Console GROM doesn't support the GROM banking concept.
// >9800 - read data        peripheral address 0
// >9802 - read address     peripheral address GROM_MODE_ADDRESS
// >9C00 - write data       peripheral address GROM_MODE_WRITE
// >9C02 - write address    peripheral address GROM_MODE_WRITE|GROM_MODE_ADDRESS
//
// GROMS have their own 13-bit internal address bus and only respond when they are
// being addressed, as determined by the top three addresses bits (which they
// apparently also remember, but are not part of the incremented register).
// In addition, real TI GROMs only contain 6k, though reside within an 8k
// window. The additional 2k, if accessed, tends to contain a bizarre OR-ing
// of bytes that implies the internal structure is a 2k ROM plus a 4k ROM,
// which certainly makes more sense than 6k. If the end of a GROM is read, the
// address counter wraps around but the match bits do not change, causing
// the memory to wrap around to the beginning of the current GROM.
// 
// Although each chip has its own address counter, they are all attached to
// the same bus, so there is technically only ONE address counter. Technically,
// multiple bases probably have multiple addresses, as does the pCode counter,
// but there seems no way in the TI system to access chips other than the currently
// addressed one, and changing the address updates all the prefetches, so there's
// no need to store per chip. 
// 
// TODO: bases need some thought, particularly getting console GROMs in all bases
//
// The console contains three GROMs:
// >0000 - boot and basic operating system
// >2000 - first part of TI BASIC
// >4000 - second part of TI BASIC
//

class Classic99GROM : public Classic99Peripheral {
public:
    Classic99GROM() = delete;
    Classic99GROM(Classic99System *core, const uint8_t *pDat, unsigned int datSize, unsigned int baseAddress);
    virtual ~Classic99GROM();

    // the addressing pins
    static const int GROM_MODE_ADDRESS = 1;    // else data
    static const int GROM_MODE_WRITE = 2;      // else read

    // ========== Classic99Peripheral interface ============

    // read needed, but no need for write. No IO mode here
    uint8_t read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw);
    void write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data);

    // interface code
    // there are no devices attached to the GROM
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
    bool init(int idx) override;                             // claim resources from the core system
    bool operate(double timestamp) override;				 // process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
    bool cleanup() override;                                 // release everything claimed in init, save NV data, etc

    // debug interface
    void getDebugSize(int &x, int &y) override;             // dimensions of a text mode output screen - either being 0 means none
    void getDebugWindow(char *buffer) override;             // output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
	//virtual void resetMemoryTracking() { }                // reset memory tracking, if the peripheral has any

    // save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
    int saveStateSize() override;                           // number of bytes needed to save state
    bool saveState(unsigned char *buffer) override;         // write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
    bool restoreState(unsigned char *buffer) override;      // restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize

    // addresses are from the system point of view
    // While the CPU does not get any memory or IO breakpoints,
    // it does breakpoint on PC, ST and WP settings. Or should. ;)
    // So does that mean implementing this interface?
    void testBreakpoint(bool isRead, int addr, bool isIO, int data);

    // configuration interface... not fully sussed out yet - could be in the debug system if that becomes doable
    void setWritable(int grom, bool isWrite) { bWritable[grom&7] = isWrite; }

protected:
    void LoadAdditionalData(const uint8_t *pDat, unsigned int datSize, unsigned int baseAddress);

private:
    void IncrementGROMAddress(int &adrRef);

    bool bWritable[8];                      // configuration option only - 8 GROM chips

    // shared data across all GROMs - probably needs some thought for bases...
    static uint8_t GROMDATA[64*1024];       // one distinct GROM space
    static int GRMADD;	                    // GROM Address counters - shared over all instances
	static int grmaccess,grmdata;			// GROM Prefetch emulation, also shared (see notes above)
    static int LastRead, LastBase;          // Reports last access for debug (also shared)
};

#endif
