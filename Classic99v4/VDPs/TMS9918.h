// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// Implementation of the TMS9918 VDP for Classic99

// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef TMS9918_H
#define TMS9918_H

#include <allegro5/allegro.h>
#include <allegro5/threads.h>
#include <atomic>
#include "../EmulatorSupport/System.h"
#include "../EmulatorSupport/peripheral.h"

class TMS9918 : public Classic99Peripheral {
public:
    TMS9918(Classic99System *core) 
        : Classic99Peripheral(core) 
    {
    }
    virtual ~TMS9918() 
    {
    };
    TMS9918() = delete;

    // you absolutely can read and write the TMS9918
    virtual uint8_t read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) override;
    virtual void write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) override;

    // interface code
    //virtual int hasAvailablePorts() { return 0; }    // number of pluggable ports (for serial, parallel, etc)
    //virtual int usesPlugPorts() { return 0; }   // number of ports that it has for attaching to (can't see this being more than 1)
    //virtual PORTTYPE getAvailablePort(int idx) { (void)idx; return NULL_PORT; }     // type of port exposed to connect to
    //virtual PORTTYPE getPlugPort(int idx) { (void)idx; return NULL_PORT; }          // type of port we need to plug into

    // send or receive a byte (or word) of data to a port, false on failure - what this means is device dependent
    //virtual bool sendPortData(int portIdx, int data) { return false; }       // this calls out to a plugged device
    //virtual bool receivePortData(int portIdx, int data) { return false; }    // this receives from a plugged device (normally only AFTER that device has called send)

    // file access for disks and cartridges
    //virtual int hasFileSystems() { return 0; }          // keep life simple, normally 0 or 1
    //virtual const char* getFileMask() { return ""; }    // return a file selector mask string, windows-style with NUL separators and double-NUL at the end
    //virtual bool loadFileData(unsigned char *buffer, int address, int length) { (void)buffer; (void)address; (void)length; return false; }  // passed from core, copy data if you need it cause it's destroyed after this call, return false if something is wrong with it

    // interface
    virtual bool init(int) override;                                 // claim resources from the core system, int is index from the host system
    virtual bool operate(double timestamp) override;     // process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
    virtual bool cleanup() override;                                 // release everything claimed in init, save NV data, etc

    // debug interface
    virtual void getDebugSize(int &x, int &y) override;              // dimensions of a text mode output screen - either being 0 means none
    virtual void getDebugWindow(char *buffer) override;              // output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
    virtual void resetMemoryTracking() override;                     // reset memory tracking, if the peripheral has any

    // save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
    virtual int saveStateSize() override;                            // number of bytes needed to save state
    virtual bool saveState(unsigned char *buffer) override;          // write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
    virtual bool restoreState(unsigned char *buffer) override;       // restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize

    // addresses are from the system point of view
    virtual void testBreakpoint(bool isRead, int addr, bool isIO, int data) override;   // test breakpoints - bare system won't know our addresses

    // VDP specific stuff
    void increment_vdpadd();
    int GetRealVDP();
    void wVDPreg(uint8_t r, uint8_t v);

    // VDP status flags
    const int VDPS_INT  = 0x80;
    const int VDPS_5SPR	= 0x40;
    const int VDPS_SCOL	= 0x20;

protected:
    int SIT;									// Screen Image Table
    int CT;										// Color Table
    int PDT;									// Pattern Descriptor Table
    int SAL;									// Sprite Allocation Table
    int SDT;									// Sprite Descriptor Table
    int CTsize;									// Color Table size in Bitmap Mode
    int PDTsize;								// Pattern Descriptor Table size in Bitmap Mode

    uint8_t VDP[16*1024];						// Video RAM (16k)
    uint8_t SprColBuf[256][192];				// Sprite Collision Buffer
    uint8_t VDPMemInited[128*1024];             // initialized memory map (todo: check size)

    // TODO: heatmap debug
//    uint8_t HeatMap[256*256*3];					// memory access heatmap (red/green/blue = CPU/VDP/GROM)
    int SprColFlag;								// Sprite collision flag
#if 0
    int bF18AActive = 0;						// was the F18 activated?
    int bF18Enabled = 1;						// is it even enabled?
    int bInterleaveGPU = 1;						// whether to run the GPU and the CPU together (impedes debug - temporary option)
#endif

    int FullScreenMode=0;						// Current full screen mode
    int FilterMode=0;							// Current filter mode
    int nDefaultScreenScale=1;					// default screen scale multiplier
    int nXSize=256, nYSize=192;					// custom sizing
    int TVFiltersAvailable=0;					// Depends on whether we can load the Filter DLL
    int TVScanLines=1;							// Whether to draw scanlines or not
    int VDPDebug=0;								// When set, displays all 256 chars
    int bShowFPS=0;								// whether to show FPS
    int bEnable80Columns=1;						// Enable the beginnings of the 80 column mode - to replace someday with F18A
    int bEnable128k=0;							// disabled by default - it's a non-real-world combination of F18 and 9938, so HACK.
    #define TV_WIDTH (602+32)					// how wide is TV mode really?

    // keyboard debug - done inline like the FPS
    int bShowKeyboard=0;                        // when set, draw the keyboard debug
    extern unsigned char capslock, lockedshiftstate;
    extern unsigned char scrolllock,numlock;
    extern unsigned char ticols[8];
    extern int fJoystickActiveOnKeys;
    extern unsigned char ignorecount;
    extern unsigned char fctnrefcount,shiftrefcount,ctrlrefcount;

    // menu display
    extern int bEnableAppMode;
    extern void SetMenuMode(bool showTitle, bool showMenu);

    sms_ntsc_t tvFilter;						// TV Filter structure
    sms_ntsc_setup_t tvSetup;					// TV Setup structure
    HMODULE hFilterDLL;							// Handle to Filter DLL
    void (*sms_ntsc_init)(sms_ntsc_t *ntsc, sms_ntsc_setup_t const *setup);	// pointer to init function
    void (*sms_ntsc_blit)(sms_ntsc_t const *ntsc, unsigned int const *sms_in, long in_row_width, int in_width, int height, void *rgb_out, long out_pitch);
																		    // pointer to blit function
    void (*sms_ntsc_scanlines)(void *pFrame, int nWidth, int nStride, int nHeight);

    HMODULE hHQ4DLL;							// Handle to HQ4x DLL
    void (*hq4x_init)(void);
    void (*hq4x_process)(unsigned char *pBufIn, unsigned char *pBufOut);

    HANDLE Video_hdl[2];						// Handles for Display/Blit events
    unsigned int *framedata;					// The actual pixel data
    unsigned int *framedata2;					// Filtered pixel data
    BITMAPINFO myInfo;							// Bitmapinfo header for the DIB functions
    BITMAPINFO myInfo2;							// Bitmapinfo header for the DIB functions
    BITMAPINFO myInfo32;						// Bitmapinfo header for the DIB functions
    BITMAPINFO myInfoTV;						// Bitmapinfo header for the DIB functions
    BITMAPINFO myInfo80Col;						// Bitmapinfo header for the DIB functions
    HDC tmpDC;									// Temporary DC for StretchBlt to work from

    int redraw_needed;							// redraw flag
    int end_of_frame;							// end of frame flag (move this to tiemul.cpp, not used in VDP)
    int skip_interrupt;							// flag for some instructions TODO: WTF are these two CPU flags doing in VDP?
    int doLoadInt;								// execute a LOAD after this instruction
    Byte VDPREG[59];							// VDP read-only registers (9918A has 8, we define 9 to support 80 cols, and the F18 has 59 (!) (and 16 status registers!))
    Byte VDPS;									// VDP Status register

    // Added by RasmusM
    int F18AStatusRegisterNo = 0;				// F18A Status register number
    int F18AECModeSprite = 0;					// F18A Enhanced color mode for sprites (0 = normal, 1 = 1 bit color mode, 2 = 2 bit color mode, 3 = 3 bit color mode)
    int F18ASpritePaletteSize = 16;				// F18A Number of entries in each palette: 2, 4, 8 (depends on ECM)
    int bF18ADataPortMode = 0;					// F18A Data-port mode
    int bF18AAutoIncPaletteReg = 0;				// F18A Auto increment palette register
    int F18APaletteRegisterNo = 0;				// F18A Palette register number
    int F18APaletteRegisterData = -1;			// F18A Temporary storage of data written to palette register
    int F18APalette[64];
    // RasmusM added end

    Word VDPADD;								// VDP Address counter
    int vdpaccess;								// VDP address write flipflop (low/high)
    int vdpwroteaddress;						// VDP (instruction) countdown after writing an address (weak test)
    int vdpscanline;							// current line being processed, 0-262 (TODO: 0 is top border, not top blanking)
    Byte vdpprefetch,vdpprefetchuninited;		// VDP Prefetch
    unsigned long hVideoThread;					// thread handle
    int hzRate;									// flag for 50 or 60hz
    int Recording;								// Flag for AVI recording
    int MaintainAspect;							// Flag for Aspect ratio
    int StretchMode;							// Setting for video stretching
    int bUse5SpriteLimit;						// whether the sprite flicker is on
    bool bDisableBlank, bDisableSprite, bDisableBackground;	// other layers :)
    bool bDisableColorLayer, bDisablePatternLayer;          // bitmap only layers


};

#endif

