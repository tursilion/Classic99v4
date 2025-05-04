// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef SN76XXX_H
#define SN76XXX_H

#if 1
// TODO: this file should be ready to go, the cpp needs work
// Also, we probably need to build a speaker system to manage audio streams
// so that the core system can respond to events and fill the audio buffers
// (similar to the television system)

#include "../EmulatorSupport/peripheral.h"
#include "../EmulatorSupport/audioSrc.h"

class SN76xxx : public Classic99Peripheral, Classic99AudioSrc {
public:
    SN76xxx(Classic99System *core); 
    virtual ~SN76xxx();
    SN76xxx() = delete;

    // == Classic99AudioSrc interface ==
    void fillAudioBuffer(void *buf, int bufSize, int samples) override;

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

protected:
    // hack for now - a little DAC buffer for cassette ticks and CPU modulation
    // fixed size for now
    bool enableBackgroundHum;    // TODO: used to be external - enable hum emulation
    double CRU_TOGGLES;          // TODO: used to be external - count CRU keyboard toggles to generate hum - interestingdata?
    double nDACLevel;            // TODO: used to be external - tape input or audio gate emulation for samples
    unsigned char dac_buffer[1024*1024];
    int dac_pos;
    double dacupdatedistance;
    double dacramp;        // a little slider to ramp in the DAC volume so it doesn't click so loudly at reset

    // value to fade every clock/16
    // init value (0.001/9.0) compares with the recordings I made of the noise generator back in the day
    // This is good, but why doesn't this match the math above, though?
    double FADECLKTICK;

    int nClock;				// NTSC, PAL may be at 3546893? - this is divided by 16 to tick
    int nCounter[4];		// 10 bit countdown timer
    int nNoisePos;			// whether noise is positive or negative (white noise only)
    unsigned short LFSR;	// noise shifter (only 15 bit)
    int nRegister[4];		// frequency registers
    int nVolume[4];			// volume attenuation
    double nFade[4];        // emulates the voltage drift back to 0 with FADECLKTICK (TODO: what does this mean with a non-zero center?)
							// we should test this against an external chip with a clean circuit.
    int max_volume;

    // audio
    int AudioSampleRate;    // in hz

    // The tapped bits are XORd together for the white noise
    // generator feedback.
    // These are the BBC micro version/Coleco? Need to check
    // against a real TI noise generator. 
    // Init value 0x0003 matches what MAME uses (although MAME shifts the other way ;) )
    int nTappedBits;
    int latch_byte;         // latched byte

    double nOutput[4];      // output scale
    double nVolumeTable[16];

private:
    void resetNoise();
    int parity(int val);
    void sound_init(int freq);
    void SetSoundVolumes();
    void MuteAudio();
    void resetDAC();

    std::mutex *csAudioBuf;
    std::shared_ptr<autoStream> stream;
};

#endif

#endif
