// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef SN76XXX_H
#define SN76XXX_H

#if 0
// TODO: this file should be ready to go, the cpp needs work
// Also, we probably need to build a speaker system to manage audio streams
// so that the core system can respond to events and fill the audio buffers
// (similar to the television system)

#include "../EmulatorSupport/peripheral.h"

class SN76xxx : public Classic99Peripheral {
public:
    SN76xxx(Classic99System *core); 
    virtual ~SN76xxx();
    SN76xxx() = delete;

    // == Classic99Peripheral interface ==

    // write-only
    //uint8_t read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw);
    void write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) override;

    // interface code
    //virtual int hasAvailablePorts() { return 0; }    // number of pluggable ports (for serial, parallel, etc)
    //virtual int usesPlugPorts() { return 0; }   // number of ports that it has for attaching to (can't see this being more than 1)
    //virtual PORTTYPE getAvailablePort(int idx) { (void)idx; return NULL_PORT; }     // type of port exposed to connect to
    //virtual PORTTYPE getPlugPort(int idx) { (void)idx; return NULL_PORT; }          // type of port we need to plug into

    // send or receive a byte (or word) of data to a port, false on failure - what this means is device dependent
    //virtual bool sendPortData(int portIdx, int data) { return false; }       // this calls out to a plugged device
    //virtual bool receivePortData(int portIdx, int data) { return false; }    // this receives from a plugged device (normally only AFTER that device has called send)

    // file access for disks and cartridges
    //virtual int hasFileSystems() { return 0; }        // keep life simple, normally 0 or 1
    //virtual const char* getFileMask() { return ""; }  // return a file selector mask string, windows-style with NUL separators and double-NUL at the end
    //virtual bool loadFileData(unsigned char *buffer, int address, int length) { (void)buffer; (void)address; (void)length; return false; }  // passed from core, copy data if you need it cause it's destroyed after this call, return false if something is wrong with it

    // interface
    bool init(int) override;                            // claim resources from the core system, int is index from the host system
    bool operate(double timestamp) override;            // process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
    bool cleanup() override;                            // release everything claimed in init, save NV data, etc

    // debug interface
    void getDebugSize(int &x, int &y) override;         // dimensions of a text mode output screen - either being 0 means none
    void getDebugWindow(char *buffer) override;         // output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
    //virtual void resetMemoryTracking() { }            // reset memory tracking, if the peripheral has any

    // save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
    int saveStateSize() override;                       // number of bytes needed to save state
    bool saveState(unsigned char *buffer) override;     // write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
    bool restoreState(unsigned char *buffer) override;  // restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize

    // addresses are from the system point of view
    //virtual void testBreakpoint(bool isRead, int addr, bool isIO, int data);              // default class will do unless the device needs to be paging or such (calls system breakpoint function if set)

};

#endif

#endif
