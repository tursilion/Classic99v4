// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef TMS9900_H
#define TMS9900_H

// Let's see what a CPU needs
class TMS9900 : public Classic99Peripheral {
public:
    TMS9900() {
        myName = "TMS9900";
    }
    virtual ~TMS9900() {
    };

    // ========== Classic99Peripheral interface ============

    // CPU has nothing to read or write
    //virtual int read(int addr, bool isIO, bool allowSideEffects) { (void)addr; (void)allowSideEffects; return 0; }
    //virtual void write(int addr, bool isIO, bool allowSideEffects, int data) { (void)addr; (void)allowSideEffects; (void)data; }

    // CPU does not have any external ports nor plug into any
    //virtual int hasAvailablePorts() { return 0; }    // number of pluggable ports (for serial, parallel, etc)
    //virtual int usesPlugPorts() { return 0; }   // number of ports that it has for attaching to (can't see this being more than 1)
    //virtual PORTTYPE getAvailablePort(int idx) { (void)idx; return NULL_PORT; }     // type of port exposed to connect to
    //virtual PORTTYPE getPlugPort(int idx) { (void)idx; return NULL_PORT; }          // type of port we need to plug into
    //virtual bool sendPortData(int portIdx, int data) { return false; }       // this calls out to a plugged device
    //virtual bool receivePortData(int portIdx, int data) { return false; }    // this receives from a plugged device (normally only AFTER that device has called send)

    // CPU does not need to load any files
    //virtual int hasFileSystems() { return 0; }          // keep life simple, normally 0 or 1
    //virtual const char* getFileMask() { return ""; }    // return a file selector mask string, windows-style with NUL separators and double-NUL at the end
    //virtual bool loadFileData(unsigned char *buffer, int address, int length) { return false; }            // passed from core, copy data if you need it cause it's destroyed after this call, return false if something is wrong with it

    // interface
    virtual bool init();                                // claim resources from the core system and initialize anything needed
    virtual bool operate(unsigned long long timestamp); // process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
    virtual bool cleanup();                             // release everything claimed in init, save NV data, etc

    // debug interface
    virtual void getDebugSize(int &x, int &y);          // dimensions of a text mode output screen - either being 0 means none
    virtual void getDebugWindow(char *buffer);          // output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings

    // CPU probably doesn't have much for breakpoints - maybe we can add status registers or something later...
    //virtual void testBreakpoint(bool isRead, int addr, bool isIO, int data);

    // save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
    virtual int saveStateSize();                        // number of bytes needed to save state
    virtual bool saveState(unsigned char *buffer);      // write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
    virtual bool restoreState(unsigned char *buffer);   // restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize

    // ========== TMS9900 functions ============


protected:
    // no special locking
    //virtual void lock()
    //virtual void unlock()

private:

};


#endif
