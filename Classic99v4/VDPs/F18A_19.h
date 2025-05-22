// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// Implementation of the F18A version 1.9 for Classic99

// TODO: literally no work has been done on this yet - this should be derived from TMS9918

// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef F18A19_H
#define F18A19_H

#if 0

#include <allegro5/allegro.h>
#include <allegro5/threads.h>
#include <atomic>
#include "../EmulatorSupport/System.h"
#include "../EmulatorSupport/peripheral.h"

// The video display is 27 + 192 + 24 lines (243 lines, with 19 lines of blanking for a total of 262 lines)
// Width is 13 + 256 + 15 = 284 pixels (plus 58 pixels of blanking for a total of 342 pixels)
// Text mode is 19 + 240 + 25 = 284 pixels still.
// F18A has a double-clock mode for 80 columns, so that would be 568 pixels
#define TMS_WIDTH 284
#define TMS_HEIGHT 243

#define TMS_DISPLAY_HEIGHT 192
#define TMS_DISPLAY_WIDTH 256
#define TMS_DISPLAY_TEXT 240

#define TMS_FIRST_DISPLAY_LINE 27
#define TMS_FIRST_DISPLAY_PIXEL 13
#define TMS_FIRST_DISPLAY_TEXT 19

class TMS9918 : public Classic99Peripheral {
public:
    TMS9918(Classic99System *core) 
        : Classic99Peripheral(core)
        , pDisplay(nullptr)
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
    virtual void getDebugSize(int &x, int &y, int user) override;              // dimensions of a text mode output screen - either being 0 means none
    virtual void getDebugWindow(char *buffer, int user) override;              // output the current debug information into the buffer, sized x*y - must include nul termination on each line
    //void debugKey(int ch, int user) override;               // receive a keypress
    virtual void resetMemoryTracking() override;                     // reset memory tracking, if the peripheral has any

    // save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
    virtual int saveStateSize() override;                            // number of bytes needed to save state
    virtual bool saveState(unsigned char *buffer) override;          // write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
    virtual bool restoreState(unsigned char *buffer) override;       // restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize

    // addresses are from the system point of view
    // As a convention, registers are stored starting at address 0x8000 FOR BREAKPOINT USE ONLY
    // This is the same as the address used to write them, so it should work out
    //virtual void testBreakpoint(bool isRead, int addr, bool isIO, int data) override;

    // VDP specific stuff
    void increment_vdpadd();
    int GetRealVDP();
    void wVDPreg(uint8_t r, uint8_t v);

    // VDP status flags
    const int VDPS_INT  = 0x80;
    const int VDPS_5SPR	= 0x40;
    const int VDPS_SCOL	= 0x20;

protected:
    int gettables(int isLayer2);
    void vdpReset(bool isCold);
    int getCharsPerLine();
    char VDPGetChar(int x, int y, int width, int height);
    void VDPdisplay(int scanline);
    void VDPgraphics(int scanline, int isLayer2);
    void VDPgraphicsII(int scanline, int isLayer2);
    void VDPtext(int scanline, int isLayer2);
    void VDPtextII(int scanline, int isLayer2);
    void VDPtext80(int scanline, int isLayer2);
    void VDPillegal(int scanline, int isLayer2);
    void VDPmulticolor(int scanline, int isLayer2);
    void VDPmulticolorII(int scanline, int isLayer2);
    unsigned int* drawTextLine(uint32_t *pDat, const char *buf);
    void DrawSprites(int scanline);

    int SIT;									// Screen Image Table
    int CT;										// Color Table
    int PDT;									// Pattern Descriptor Table
    int SAL;									// Sprite Allocation Table
    int SDT;									// Sprite Descriptor Table
    int CTsize;									// Color Table size in Bitmap Mode
    int PDTsize;								// Pattern Descriptor Table size in Bitmap Mode

    uint8_t VDP[16*1024];						// Video RAM (16k)
    uint8_t SprColBuf[256];     				// Sprite Collision Buffer
    uint8_t VDPMemInited[128*1024];             // initialized memory map (todo: check size)

    // TODO: heatmap debug
//    uint8_t HeatMap[256*256*3];					// memory access heatmap (red/green/blue = CPU/VDP/GROM)
    int SprColFlag;								// Sprite collision flag

#if 0
    int bF18AActive = 0;						// was the F18 activated?
    int bF18Enabled = 1;						// is it even enabled?
    int bInterleaveGPU = 1;						// whether to run the GPU and the CPU together (impedes debug - temporary option)
#endif

    int VDPDebug=0;								// When set, displays all 256 chars
    int bShowFPS=0;								// whether to show FPS
    int bEnable80Columns=1;						// Enable the beginnings of the 80 column mode - to replace someday with F18A
    int bEnable128k=0;							// disabled by default - it's a non-real-world combination of F18 and 9938, so HACK.

    // menu display
//    extern int bEnableAppMode;
//    extern void SetMenuMode(bool showTitle, bool showMenu);

    uint32_t *pLine;                            // pointer to the start of the currently locked scanline

    int redraw_needed;							// redraw flag
    int skip_interrupt;							// flag for some instructions TODO: WTF are these two CPU flags doing in VDP?
    int doLoadInt;								// execute a LOAD after this instruction
    int VDPREG[59]; 							// VDP read-only registers (9918A has 8, we define 9 to support 80 cols, and the F18 has 59 (!) (and 16 status registers!))
    int VDPS;									// VDP Status register

    // Added by RasmusM
    int F18AStatusRegisterNo = 0;				// F18A Status register number
    int F18AECModeSprite = 0;					// F18A Enhanced color mode for sprites (0 = normal, 1 = 1 bit color mode, 2 = 2 bit color mode, 3 = 3 bit color mode)
    int F18ASpritePaletteSize = 16;				// F18A Number of entries in each palette: 2, 4, 8 (depends on ECM)
    int bF18ADataPortMode = 0;					// F18A Data-port mode
    int bF18AAutoIncPaletteReg = 0;				// F18A Auto increment palette register
    int F18APaletteRegisterNo = 0;				// F18A Palette register number
    int F18APaletteRegisterData = -1;			// F18A Temporary storage of data written to palette register
    Color F18APalette[64];
    // RasmusM added end

    int VDPADD;								    // VDP Address counter
    int vdpaccess;								// VDP address write flipflop (low/high)
    int vdpwroteaddress;						// VDP (instruction) countdown after writing an address (weak test)
    int vdpscanline;							// current line being processed, 0-262 (TODO: 0 is top border, not top blanking)
    int vdpprefetch,vdpprefetchuninited;		// VDP Prefetch
    unsigned long hVideoThread;					// thread handle
    int hzRate;									// flag for 50 or 60hz
    int Recording;								// Flag for AVI recording
    int MaintainAspect;							// Flag for Aspect ratio
    int StretchMode;							// Setting for video stretching
    int bUse5SpriteLimit;						// whether the sprite flicker is on
    bool bDisableBlank, bDisableSprite, bDisableBackground;	// other layers :)
    bool bDisableColorLayer, bDisablePatternLayer;          // bitmap only layers

    std::shared_ptr<autoBitmap> pDisplay;       // allocated display - always 284x243 for 9918, so we only need one layer
};

#endif

#endif

