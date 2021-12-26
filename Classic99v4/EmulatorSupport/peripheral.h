// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef EMULATOR_SUPPORT_PERIPHERAL_H
#define EMULATOR_SUPPORT_PERIPHERAL_H

#include <allegro5/allegro.h>
#include <allegro5/threads.h>
#include <atomic>
#include "System.h"

// we need this because this header is included from system.h before the class is defined
class Classic99System;

// TODO: we can add a "debug_write" wrapper function to this class, and this
// would allow the debug system to turn debug on or off per peripheral.

// Port emulation today is rather high level, and probably won't contain control bits
// but when we someday need them, then we'll figure it out...
enum PORTTYPE {
    NULL_PORT,

    SERIAL_PORT,
    PARALLEL_PORT,
    GAME_PORT,
    KEYBOARD_PORT,
    MOUSE_PORT
};

// object for tracking breakpoints
class BREAKPOINT {
public:
    // breakpoint mask types
    static const int BREAKPOINT_MASK_READ = 0x01;
    static const int BREAKPOINT_MASK_WRITE = 0x02;
    static const int BREAKPOINT_MASK_IO = 0x04;

    static const int BREAKPOINT_MASK_GROM = 0x08;
    static const int BREAKPOINT_MASK_PERIPHERAL = (BREAKPOINT_MASK_GROM);

    static const int BREAKPOINT_MASK_ACCESS = BREAKPOINT_MASK_READ | BREAKPOINT_MASK_WRITE;
    static const int BREAKPOINT_MASK_READ_IO = BREAKPOINT_MASK_READ | BREAKPOINT_MASK_IO;
    static const int BREAKPOINT_MASK_WRITE_IO = BREAKPOINT_MASK_WRITE | BREAKPOINT_MASK_IO;
    static const int BREAKPOINT_MASK_ACCESS_IO = BREAKPOINT_MASK_IO | BREAKPOINT_MASK_ACCESS;

    BREAKPOINT() :
        typeMask(0), page(0), addressA(0), addressB(0), dataMask(0), data(0) { 
    };
    BREAKPOINT(int inType, int inPage, int inAdrA, int inAdrB, int inMask, int inData) :
        typeMask(inType), page(inPage), addressA(inAdrA), addressB(inAdrB), dataMask(inMask), data(inData) { 
    };

    BREAKPOINT& operator=(const BREAKPOINT &other) {
        if (this == &other) {
            return *this;
        }

        typeMask = other.typeMask;
        page = other.page;
        addressA = other.addressA;
        addressB = other.addressB;
        dataMask = other.dataMask;
        data = other.data;

        return *this;
    };

    inline bool operator==(const BREAKPOINT& rhs) {
        if ((typeMask == rhs.typeMask) &&
            (page == rhs.page) && 
            (addressA == rhs.addressA) &&
            (addressB == rhs.addressB) &&
            (dataMask == rhs.dataMask) &&
            (data == rhs.data)) {
            return true;
        }
        return false;
    }
    inline bool operator!=(const BREAKPOINT& rhs) {
        return !((*this)==rhs);
    }

    bool CheckRange(int x, int inPage) {
	    // check bank first (assumes ranges are only for addresses, not data)
	    if (page != 0) {
			if (page != inPage) {
				// bank required and not the right bank
				return false;
			}
	    }

	    if (addressB) {
		    // this is a range
		    if ((x >= addressA) && (x <= addressB)) {
			    return true;
		    }
	    } else {
		    // not a range
		    if (x == addressA) {
			    return true;
		    }
	    }
	    return false;
    }

    int typeMask;   // access mask as above
    int page;       // page on peripheral, or 0
    int addressA;   // first system address, or 0
    int addressB;   // second system address, or 0 if not a range
    int dataMask;   // a mask to apply to data comparisons, or zero if ignored
    int data;       // the data to compare using dataMask
};

// per memory range, should be fine
const int MAX_BREAKPOINTS = 32;

// The base peripheral interface allows for instantiating and destroying
// a peripheral instance, as well as providing a debug interface for it

// this one is NOT pure virtual so the base class can be used as a dummy object
// Be careful! "theCore" is allowed to be NULL in the dummy object!
class Classic99Peripheral {
public:
    Classic99Peripheral(Classic99System *core) 
        : theCore(core) 
    {
        // create the lock object - we can't be certain that
        // we don't need a recursive mutex since others can use it
        periphLock = al_create_mutex_recursive();
        trackIsActive = false;
        page = 0;
        lastTimestamp = 0;
        setIndex("Dummy", 0);
    }
    virtual ~Classic99Peripheral() {
        // have the derived class clean up
        cleanup();
        // release the lock object
        al_destroy_mutex(periphLock);
    };
    Classic99Peripheral() = delete;

    // disallow copy and assign constructors
    Classic99Peripheral(Classic99Peripheral const& other) = delete;
    Classic99Peripheral &operator=(Classic99Peripheral const &other) = delete;

    // dummy read and write - IO flag is unused on most, but just in case they need to know
    virtual uint8_t read(int addr, bool isIO, volatile long &, MEMACCESSTYPE rmw) { (void)addr; (void)rmw; return 0; }
    virtual void write(int addr, bool isIO, volatile long &, MEMACCESSTYPE rmw, uint8_t data) { (void)addr; (void)rmw; (void)data; }

    // interface code
    virtual int hasAvailablePorts() { return 0; }    // number of pluggable ports (for serial, parallel, etc)
    virtual int usesPlugPorts() { return 0; }   // number of ports that it has for attaching to (can't see this being more than 1)
    virtual PORTTYPE getAvailablePort(int idx) { (void)idx; return NULL_PORT; }     // type of port exposed to connect to
    virtual PORTTYPE getPlugPort(int idx) { (void)idx; return NULL_PORT; }          // type of port we need to plug into

    // send or receive a byte (or word) of data to a port, false on failure - what this means is device dependent
    virtual bool sendPortData(int portIdx, int data) { return false; }       // this calls out to a plugged device
    virtual bool receivePortData(int portIdx, int data) { return false; }    // this receives from a plugged device (normally only AFTER that device has called send)

    // file access for disks and cartridges
    virtual int hasFileSystems() { return 0; }          // keep life simple, normally 0 or 1
    virtual const char* getFileMask() { return ""; }    // return a file selector mask string, windows-style with NUL separators and double-NUL at the end
    virtual bool loadFileData(unsigned char *buffer, int address, int length) { (void)buffer; (void)address; (void)length; return false; }  // passed from core, copy data if you need it cause it's destroyed after this call, return false if something is wrong with it

    // interface
    virtual bool init(int) { return true; }                                 // claim resources from the core system, int is index from the host system
    virtual bool operate(double timestamp) { return true; }                 // process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
    virtual bool cleanup() { return true; }                                 // release everything claimed in init, save NV data, etc

    // debug interface
    virtual void getDebugSize(int &x, int &y) { x=0; y=0; }       // dimensions of a text mode output screen - either being 0 means none
    virtual void getDebugWindow(char *buffer) { (void)buffer; }   // output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
    virtual void resetMemoryTracking() { }                        // reset memory tracking, if the peripheral has any

    // save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
    // TODO: make all saveState functions versioned (separate version per object, as well as a master version)
    // The system decides the order of the objects and initializes/restores the save file
    virtual int saveStateSize() { return 0; }       // number of bytes needed to save state
    virtual bool saveState(unsigned char *buffer) { (void)buffer; return true; }    // write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
    virtual bool restoreState(unsigned char *buffer) { (void)buffer; return true; } // restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize

    // addresses are from the system point of view
    void addBreakpoint(BREAKPOINT &inBreak);      // add a breakpoint on access to this device - should be checked for all accesses
    void removeBreakpoint(BREAKPOINT &inBreak);   // add a breakpoint on access to this device - should be checked for all accesses
    virtual void testBreakpoint(bool isRead, int addr, bool isIO, int data);              // default class will do unless the device needs to be paging or such (calls system breakpoint function if set)
    BREAKPOINT getBreakpoint(int idx);                                                    // returns a copy of the indexed breakpoint, or a breakpoint with a typeMask of 0 if invalid index (used to enumerate)

    // flags used by the core to track active peripherals during memory access
    void setActive() { trackIsActive = true; }
    volatile bool isActive() { return trackIsActive; }
    void clearActive() { trackIsActive = false; }

    // index and name creation
    // pass NULL name to ONLY change the index
    void setIndex(const char *name, int in);

    // name retrieval 
    const char *getName() { return formattedName; }

protected:
    void saveStateVal(unsigned char *&buffer, bool n);
    void saveStateVal(unsigned char *&buffer, double n);
    void saveStateVal(unsigned char *&buffer, int n);
    void saveStateVal(unsigned char *&buffer, short n);
    void saveStateVal(unsigned char *&buffer, unsigned int n);
    void saveStateVal(unsigned char *&buffer, unsigned short n);
    // prevent any implicit conversion, I want to know about it since we're
    // working with binary data
    template <class T> void saveStateVal(unsigned char *&buffer, T) = delete;

    void loadStateVal(unsigned char *&buffer, bool &n);
    void loadStateVal(unsigned char *&buffer, double &n);
    void loadStateVal(unsigned char *&buffer, int &n);
    void loadStateVal(unsigned char *&buffer, short &n);
    void loadStateVal(unsigned char *&buffer, unsigned int &n);
    void loadStateVal(unsigned char *&buffer, unsigned short &n);
    // prevent any implicit conversion, I want to know about it since we're
    // working with binary data
    template <class T> void loadStateVal(unsigned char *&buffer, T) = delete;

    ALLEGRO_MUTEX *periphLock;          // our object lock
    Classic99System *theCore;           // pointer to the core - note all periphs need to be deleted before invalidating the core!
    double lastTimestamp;               // last time we ran to
    int page;                           // a semi-opaque value used by implementations for memory paging in breakpoints
    BREAKPOINT breaks[MAX_BREAKPOINTS]; // list of device breakpoints

private:
    char formattedName[128];            // the two combined

    volatile bool trackIsActive;        // flag for the memory update - used by setActive/isActive/clearActive

    friend class Classic99System;       // allowed access to our lock methods
};

extern Classic99Peripheral dummyPeripheral;

#endif
