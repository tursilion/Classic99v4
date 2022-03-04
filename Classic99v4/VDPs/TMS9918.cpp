// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// TODO: delete the F18A stuff once we have that version of the file - saving it for now so it's easier to make that version work
// Basically, I want to get this VDP working more or less like the old Classic99 one did, then I can split it up into the 9918A
// and F18A versions/overrides and clean this one up as a base class
// TODO: VDP register write breakpoints?
// TODO: make sure this is a TMS9918 (not A) after the other variants are dropped in

#if 0
F18A Reset from Matt:

0 in all registers, except:

VR1 = >40 (no blanking, the 4K/16K bit is ignored in the F18A)
VR3 = >10 (color table at >0400)
VR4 = >01 (pattern table at >0800)
VR5 = >0A (sprite table at >0500)
VR6 = >02 (sprite pattern table at >1000)
VR7 = >1F (fg=black, bg=white)
VR30 = sprite_max (set from external jumper setting)
VR48 = 1 (increment set to 1)
VR51 = 32 (stop sprite to max)
VR54 = >40 (GPU PC MSB)
VR55 = >00 (GPU PC LBS)
VR58 = 6 (GROMCLK divider)

The real 9918A will set all VRs to 0, which basically makes the screen black, blank, and off, 4K VRAM selected, and no interrupts. 

Note that nothing restores the F18A palette registers to the power-on defaults, other than a power on.

As for the GPU, the VR50 >80 reset will *not* stop the GPU, and if the GPU code is modifying VDP registers, then it can overwrite the reset values.  However, since the reset does clear VR50, the horizontal and vertical interrupt enable bits will be cleared, and thus the GPU will not be triggered on those events.

The reset also changes VR54 and VR55, but they are *not* loaded to the GPU PC (program counter).  The only events that change the GPU PC are:

* normal GPU instruction execution.
* the external hardware reset.
* writing to VR55 (GPU PC LSB).
* writing >00 to VR56 (load GPU PC from VR54 and VR55, then idle).

#endif



#include "TMS9918.h"
#include <cstdio>
#include <ctype.h>
#include "../EmulatorSupport/peripheral.h"
#include "../EmulatorSupport/debuglog.h"
#include "../EmulatorSupport/System.h"
#include "../EmulatorSupport/interestingData.h"
#include "../EmulatorSupport/tv.h"

// 12-bit 0RGB colors (we shift up to 32 bit when we load it). 12 bit is the correct palette for the F18A.
const int F18APaletteReset[64] = {
	// standard 9918A colors
	0x0000,  // 0 Transparent
	0x0000,  // 1 Black
	0x02C3,  // 2 Medium Green
	0x05D6,  // 3 Light Green
	0x054F,  // 4 Dark Blue
	0x076F,  // 5 Light Blue
	0x0D54,  // 6 Dark Red
	0x04EF,  // 7 Cyan
	0x0F54,  // 8 Medium Red
	0x0F76,  // 9 Light Red
	0x0DC3,  // 10 Dark Yellow
	0x0ED6,  // 11 Light Yellow
	0x02B2,  // 12 Dark Green
	0x0C5C,  // 13 Magenta
	0x0CCC,  // 14 Gray
	0x0FFF,  // 15 White
	// EMC1 version of palette 0	// the rest, I think, are for the default F18A palette
	0x0000,  // 0 Black
	0x02C3,  // 1 Medium Green
	0x0000,  // 2 Black
	0x054F,  // 3 Dark Blue
	0x0000,  // 4 Black
	0x0D54,  // 5 Dark Red
	0x0000,  // 6 Black
	0x04EF,  // 7 Cyan
	0x0000,  // 8 Black
	0x0CCC,  // 9 Gray
	0x0000,  // 10 Black
	0x0DC3,  // 11 Dark Yellow
	0x0000,  // 12 Black9
	0x0C5C,  // 13 Magenta
	0x0000,  // 14 Black
	0x0FFF,  // 15 White
	// IBM CGA colors
	0x0000,  // 0 >000000 ( 0 0 0) black
	0x000A,  // 1 >0000AA ( 0 0 170) blue
	0x00A0,  // 2 >00AA00 ( 0 170 0) green
	0x00AA,  // 3 >00AAAA ( 0 170 170) cyan
	0x0A00,  // 4 >AA0000 (170 0 0) red
	0x0A0A,  // 5 >AA00AA (170 0 170) magenta
	0x0A50,  // 6 >AA5500 (170 85 0) brown
	0x0AAA,  // 7 >AAAAAA (170 170 170) light gray
	0x0555,  // 8 >555555 ( 85 85 85) gray
	0x055F,  // 9 >5555FF ( 85 85 255) light blue
	0x05F5,  // 10 >55FF55 ( 85 255 85) light green
	0x05FF,  // 11 >55FFFF ( 85 255 255) light cyan
	0x0F55,  // 12 >FF5555 (255 85 85) light red
	0x0F5F,  // 13 >FF55FF (255 85 255) light magenta
	0x0FF5,  // 14 >FFFF55 (255 255 85) yellow
	0x0FFF,  // 15 >FFFFFF (255 255 255) white
	// ECM1 version of palette 2
	0x0000,  // 0 >000000 ( 0 0 0) black
	0x0555,  // 1 >555555 ( 85 85 85) gray
	0x0000,  // 2 >000000 ( 0 0 0) black
	0x000A,  // 3 >0000AA ( 0 0 170) blue
	0x0000,  // 4 >000000 ( 0 0 0) black
	0x00A0,  // 5 >00AA00 ( 0 170 0) green
	0x0000,  // 6 >000000 ( 0 0 0) black
	0x00AA,  // 7 >00AAAA ( 0 170 170) cyan
	0x0000,  // 8 >000000 ( 0 0 0) black
	0x0A00,  // 9 >AA0000 (170 0 0) red
	0x0000,  // 10 >000000 ( 0 0 0) black
	0x0A0A,  // 11 >AA00AA (170 0 170) magenta
	0x0000,  // 12 >000000 ( 0 0 0) black
	0x0A50,  // 13 >AA5500 (170 85 0) brown
	0x0000,  // 14 >000000 ( 0 0 0) black
	0x0FFF   // 15 >FFFFFF (255 255 255) white
};

// watch length, these need to fit doubled up in the debug window
static const char *COLORNAMES[16] = {
	"Transparent",
	"Black",
	"Med Green",
	"Lt Green",
	"Dk Blue",
	"Lt Blue",
	"Dk Red",
	"Cyan",
	"Med Red",
	"Lt Red",
	"Dk Yellow",
	"Lt Yellow",
	"Dk Green",
	"Magenta",
	"Gray",
	"White"
};

const char *digpat[10][5] = {
{
"111",
"101",
"101",
"101",
"111"
},
{
"010",
"110",
"010",
"010",
"111"
},
{
"111",
"001",
"111",
"100",
"111"
},
{
"111",
"001",
"011",
"001",
"111"
},
{
"101",
"101",
"111",
"001",
"001"
},
{
"111",
"100",
"111",
"001",
"111"
},
{
"111",
"100",
"111",
"101",
"111"
},
{
"111",
"001",
"001",
"010",
"010"
},
{
"111",
"101",
"111",
"101",
"111"
},
{
"111",
"101",
"111",
"001",
"111"
}
};

// some local defines
#define REDRAW_LINES 262

// ARGB format
#define HOT_PINK_TRANS 0x00ff00ff

// So the TMS9918 only has two addresses - zero for data and 1 for registers

// TODO: create a "InterestingData" that contains a list of information that other
// systems want, probably just an array with an indexing enum for speed. One
// system ONLY writes to that database, and anyone is allowed to read it. Boom, done.
// Information can be system specific, if I'm sure, or generalized. For instance, when
// the VDP is checking for interrupts to be enabled, it really only needs to know
// "MAIN_CPU_INTERRUPTS_ENABLED", it doesn't need to know if it's a TMS9900.
// We can also store the system enum in that database for system specific hacks...
// for example, the ColecoVision VDP is on the NMI, so there's no point it checking
// for MAIN_CPU_INTERRUPTS_ENABLED, cause it's irrelevant. For performance, we need
// an easy way to see if something is NOT in the database, which is a quick way
// of indicating that the value is irrelevant in the current configuration. Maybe -1
// is sufficient for all reasonable values...
// Anyway, don't need to cover all possible cases, only the cases we care about.

// return true if our interrupt line is active
bool TMS9918::isIntActive() {
	if ((VDPS&VDPS_INT) && (VDPREG[1] & 0x20)) {
		return true;
	}
	return false;
}

// read request from the system
// cycles is the cycles used so far on this operation, and should be
// updated if we consume any extra (VDP doesn't, but we do use this to catch up the VDP for status reads)
uint8_t TMS9918::read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) {
    // address 0 - read data
    // address 1 - read status
    // IO: don't care (but passed to breakpoint test)
    // cycles - no additional cycles added, but we do need the existing count
    // rmw - direct access when free

    if (rmw != ACCESS_FREE) {
        if (getInterestingDataIndirect(INDIRECT_MAIN_CPU_INTERRUPTS_ENABLED) == DATA_TRUE) {
            debug_write("Warning: PC >%04X reading VDP with interrupts enabled.", getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC));
        }
    }

    if (rmw == ACCESS_FREE) {
        // this is the emulator or debugger wanting access to memory, don't
        // treat it as a peripheral access
        return VDP[addr&0x3fff];
    } else if (addr) {	
        /* read status */
        if (rmw != ACCESS_FREE) {
            // make sure the VDP is up to date with the current count
		    // This accomodates code that requires the VDP state to be able to change
		    // DURING an instruction (the old code, the VDP state and the VDP interrupt
		    // would both change and be recognized between instructions). This allows 
            // Lee's fbForth random number code to function, which worked by watching 
            // for the interrupt bit while leaving interrupts enabled.
            // Safe to just call, we know we can't be recursed.
			// TODO: 'cycles' is reset every instruction, so we can't use it to
			// determine how far along the CPU is. But we probably should
			// That said, a fake value in here should be adequate?
            //operate(lastTimestamp+cycles);
			operate(lastTimestamp+64);	// about a scanline's worth
        }

#if 0 
		// The F18A turns off DPM if any status register is read
		if ((bF18AActive)&&(bF18ADataPortMode)) {
			bF18ADataPortMode = 0;
			F18APaletteRegisterNo = 0;
			debug_write("F18A Data port mode off (status register read).");
		}

		// Added by RasmusM
		if ((bF18AActive) && (F18AStatusRegisterNo > 0)) {
			return getF18AStatus();
		}
		// RasmusM added end
#endif

        uint8_t ret = VDPS; // does not affect prefetch or address (tested on hardware)
		VDPS&=0x1f;			// top flags are cleared on read (tested on hardware)
		vdpaccess=0;		// reset byte flag

		// TODO: hack to make Miner2049 work. If we are reading the status register mid-frame,
		// and neither 5S or F are set, return the highest sprite index for the scanline. It
		// could in theory randomly be lower, but this seems to work. (Miner2049 cares about bit 0x02)
        // When this is wrong, Miner2049er never detects sprite collisions ;)
		// And put it in the draw code, not here?
		if ((ret&(VDPS_5SPR|VDPS_INT)) == 0) {
			// This search code borrowed from the sprite draw code
			int highest=31;
			int SAL=((VDPREG[5]&0x7f)<<7);

			// find the highest active sprite
			for (int i1=0; i1<32; i1++)			// 32 sprites 
			{
				if (VDP[SAL+(i1<<2)]==0xd0)
				{
					highest=i1-1;
					break;
				}
			}
			if (highest > 0) {
				ret=(ret&0xe0)|(highest);
			}
		}

		return(ret);
    } else {
        /* read data */
		int RealVDP;

#if 0
		if (vdpwroteaddress > 0) {
			// todo: need some defines - 0 is top border, not top blanking
            // todo more: going to redo all the frame buffer anyway...
			if ((vdpscanline >= 13) && (vdpscanline < 192+13) && (VDPREG[1]&0x40)) {
				debug_write("Warning - may be reading VDP too quickly after address write at >%04X!", getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC));
				vdpwroteaddress = 0;
			}
		}
#endif

		vdpaccess=0;		// reset byte flag (confirmed in hardware)
		RealVDP = GetRealVDP();
        // TODO: heatmap debug
//		UpdateHeatVDP(RealVDP);

        // check uninitialized memory
		if ((getInterestingData(DATA_CHECK_UNINITIALIZED_MEMORY) == DATA_TRUE) && (vdpprefetchuninited)) {
			// we have to remember if the prefetch was initted, since there are other things it could have
			debug_write("Breakpoint - PC >%04X reading uninitialized VDP memory at >%04X (or other prefetch)", 
						getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC), (RealVDP-1)&0x3fff);
			theCore->triggerBreakpoint();
		}

        if (rmw == ACCESS_READ) {
            // address-1 because of prefetch, and vdpprefetch is the data
            testBreakpoint(true, VDPADD-1, isIO, vdpprefetch);
		}

		uint8_t ret = vdpprefetch;
		vdpprefetch = VDP[RealVDP];
		vdpprefetchuninited = (VDPMemInited[RealVDP] == 0);
   		increment_vdpadd();

		return ret;
	}
}

// write request from the system
void TMS9918::write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) {
    // address 0 - write data
    // address 1 - write address
    // IO: don't care (but passed to breakpoint test)
    // cycles - no additional cycles added, but we do need the existing count
    // rmw - direct access when free

	int RealVDP;

    if (rmw != ACCESS_FREE) {
        if (getInterestingDataIndirect(INDIRECT_MAIN_CPU_INTERRUPTS_ENABLED) == DATA_TRUE) {
            debug_write("Warning: PC >%04X writing VDP with CPU interrupts enabled", getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC));
        }
    }

    if (rmw == ACCESS_FREE) {
		// direct access, probably emulation, so just do it
        VDP[addr&0x3fff] = data;
    } else if (addr) {
		/* write address */

		if (0 == vdpaccess) {
			// LSB (confirmed in hardware)
			VDPADD = (VDPADD & 0xff00) | data;
			vdpaccess = 1;
		} else {
			// MSB - flip-flop is reset and triggers action (confirmed in hardware)
			VDPADD = (VDPADD & 0x00FF) | (data<<8);
			vdpaccess = 0;

#if 0
			// Setting a countdown timer in cycles before it's safe to access the VDP
			// TODO: get this right - we have actual microsecond cycles available now, sort of...
			// we don't get reads predictably. Maybe access to the central system clock?
			// count down access cycles to help detect write address/read vdp overruns (there may be others but we don't think so!)
			// anyway, we need 8uS or we warn
			if (max_cpf > 0) {
				// TODO: this is still wrong. Since the issue is mid-instruction, we need to
				// count this down either at each phase, or calculate better what it needs
				// to be before the read. Where this is now, it will subtract the cycles from
				// this instruction, when it probably shouldn't. The write to the VDP will
				// happen just 4 (5?) cycles before the end of the instruction (multiplexer)
				// TODO: if we use the central microsecond clock, then we know "right now" is
				// that time plus cycles microseconds, so we are pretty close, except for the
				// current instruction. There is always going to be some slop without mid-instruction
				// timing...
				if (hzRate == HZ50) {
					vdpwroteaddress = (HZ50 * max_cpf) * 8 / 1000000;
				} else {
					vdpwroteaddress = (HZ60 * max_cpf) * 8 / 1000000;
				}
			}
#endif

			// We /could/ just add the specific words to the interesting data database... would not impact much
			if (getInterestingData(DATA_SYSTEM_TYPE) == SYSTEM_TI99) {
				// check if the user is probably trying to do DSR filename tracking
				// This is a TI disk controller side effect and shouldn't be relied
				// upon - particularly since it involved investigating freed buffers ;)
				if (((VDPADD == 0x3fe1)||(VDPADD == 0x3fe2)) && (getInterestingData(DATA_TMS9900_8356) == 0x3fe1)) {
					debug_write("Software may be trying to track filenames using deleted TI VDP buffers... (>8356)");
					if (getInterestingData(DATA_BREAK_ON_DISK_CORRUPT) == DATA_TRUE) theCore->triggerBreakpoint();
				}
			}

            // check what to do with the write
			if (VDPADD&0x8000) { 
                // register write
				int nReg = (VDPADD&0x3f00)>>8;
				int nData = VDPADD&0xff;

#if 0
				if (bF18Enabled) {
					if ((nReg == 57) && (nData == 0x1c)) {
						// F18A unlock sequence? Supposed to be twice but for now we'll just take equal
						// TODO: that's hacky and it's wrong. Fix it. 
						if (VDPREG[nReg] == nData) {	// but wait -- isn't this already verifying twice? TODO: Double-check procedure
							bF18AActive = true;
							debug_write("F18A Enhanced registers unlocked.");
						} else {
							VDPREG[nReg] = nData;
						}
						return;
					}
				} else {
					// this is hacky ;)
					bF18AActive = false;
				}
				if (bF18AActive) {
					// check extended registers. 
					// TODO: the 80 column stuff below should be included in the F18 specific stuff, but it's not right now

					// The F18 has a crapload of registers. But I'm only interested in a few right now, the rest can fall through

					// TODO: a lot of these side effects need to move to wVDPReg
					// Added by RasmusM
					if (nReg == 15) {
						// Status register select
						//debug_write("F18A status register 0x%02X selected", nData & 0x0f);
						F18AStatusRegisterNo = nData & 0x0f;
						return;
					}
					if (nReg == 47) {
						// Palette control
						bF18ADataPortMode = (nData & 0x80) != 0;
						bF18AAutoIncPaletteReg = (nData & 0x40) != 0;
						F18APaletteRegisterNo = nData & 0x3f;
						F18APaletteRegisterData = -1;
						if (bF18ADataPortMode) {
							debug_write("F18A Data port mode on.");
						}
						else {
							debug_write("F18A Data port mode off.");
						}
						return;
					}
					if (nReg == 49) {
                        VDPREG[nReg] = nData;

                        // Enhanced color mode
						F18AECModeSprite = nData & 0x03;
						F18ASpritePaletteSize = 1 << F18AECModeSprite;	
						debug_write("F18A Enhanced Color Mode 0x%02X selected for sprites", nData & 0x03);
						// TODO: read remaining bits: fixed tile enable, 30 rows, ECM tiles, real sprite y coord, sprite linking. 
						return;
					}
					// RasmusM added end

                    if (nReg == 50) {
                        VDPREG[nReg] = nData;
                        // TODO: other reg50 bits
                        // 0x01 - Tile Layer 2 uses sprite priority (when 0, always on top of sprites)
                        // 0x02 - use per-position attributes instead of per-name in text modes (DONE)
                        // 0x04 - show virtual scanlines (regardless of jumper)
                        // 0x08 - report max sprite (VR30) instead of 5th sprite
                        // 0x10 - disable all tile 1 layers (GM1,GM2,MCM,T40,T80)
                        // 0x20 - trigger GPU on VSYNC
                        // 0x40 - trigger GPU on HSYNC
                        // 0x80 - reset VDP registers to poweron defaults (DONE)
                        if (nReg & 0x80) {
                            debug_write("Resetting F18A registers");
                            vdpReset(false);
                        }
                    }

					if (nReg == 54) {
						// GPU PC MSB
						VDPREG[nReg] = nData;
						return;
					}
					if (nReg == 55) {
						// GPU PC LSB -- writes trigger the GPU
						VDPREG[nReg] = nData;
						pGPU->SetPC((VDPREG[54]<<8)|VDPREG[55]);
						debug_write("GPU PC LSB written, starting GPU at >%04X", pGPU->GetPC());
						pGPU->StopIdle();
						if (!bInterleaveGPU) {
							pCurrentCPU = pGPU;
						}
						return;
					}
					if (nReg == 56) {
						// GPU control register
						if (nData & 0x01) {
							if (pGPU->idling) {
								// going to run the code anyway to be sure, but only debug on transition
								debug_write("GPU GO bit written, starting GPU at >%04X", pGPU->GetPC());
							}
							pGPU->StopIdle();
							if (!bInterleaveGPU) {
								pCurrentCPU = pGPU;
							}
						} else {
							if (!pGPU->idling) {
								debug_write("GPU GO bit cleared, stopping GPU at >%04X", pGPU->GetPC());
							}
							pGPU->StartIdle();
							pCurrentCPU = pCPU;		// probably redundant
						}
						return;
					}
				}

				if (bEnable80Columns) {
					// active only when 80 column is enabled
					// special hack for RAM... good lord.
					if ((bEnable128k) && (nReg == 14)) {
						VDPREG[nReg] = nData&0x07;
						redraw_needed=REDRAW_LINES;
						return;
					}

				}

				if (bF18AActive) {
					wVDPreg((Byte)(nReg&0x3f),(Byte)(nData));
				} else
#endif
                {
                    // don't want to return anyway...
					if (nReg&0xf8) {
						debug_write("Warning: writing >%02X to VDP register >%X ends up at >%Xd (PC=>%04X)", 
									nData, nReg, nReg&0x07, getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC));
					}
					// verified correct against real hardware - register is masked to 3 bits
					wVDPreg((uint8_t)(nReg&0x07),nData);
				}
				redraw_needed=REDRAW_LINES;
			}

			// And the address remains set even when the target is a register
			if ((VDPADD&0xC000)==0) {	// prefetch inhibit? Verified on hardware - either bit inhibits.
				RealVDP = GetRealVDP();
				vdpprefetch=VDP[RealVDP];
				vdpprefetchuninited = (VDPMemInited[RealVDP] == 0);
				increment_vdpadd();
			} else {
				VDPADD&=0x3fff;			// writing or register, just mask the bits off
			}
		}
		// verified on hardware - write register does not update the prefetch buffer
	}
	else
	{	/* write data */
#if 0
		// TODO: cold reset incompletely resets the F18 state - after TI Scramble runs we see a sprite on the master title page
        // Added by RasmusM
		// Write data to F18A palette registers
		if (bF18AActive && bF18ADataPortMode) {
			if (F18APaletteRegisterData == -1) {
				// Read first byte
				F18APaletteRegisterData = c;
			}
			else {
				// Read second byte
				{
					int r=(F18APaletteRegisterData & 0x0f);
					int g=(c & 0xf0)>>4;
					int b=(c & 0x0f);
					F18APalette[F18APaletteRegisterNo] = (r<<20)|(r<<16)|(g<<12)|(g<<8)|(b<<4)|b;	// double up each palette gun, suggestion by Sometimes99er
					redraw_needed = REDRAW_LINES;
				}
				debug_write("F18A palette register >%02X set to >%04X", F18APaletteRegisterNo, F18APalette[F18APaletteRegisterNo]);
				if (bF18AAutoIncPaletteReg) {
					F18APaletteRegisterNo++;
				}
				// The F18A turns off DPM after each register is written if auto increment is off
				// or after writing to last register if auto increment in on
				if ((!bF18AAutoIncPaletteReg) || (F18APaletteRegisterNo == 64)) {
					bF18ADataPortMode = 0;
					F18APaletteRegisterNo = 0;
					debug_write("F18A Data port mode off (auto).");
				}
				F18APaletteRegisterData = -1;
			}
            redraw_needed=REDRAW_LINES;
			return;
		}
		// RasmusM added end
#endif

		vdpaccess=0;		// reset byte flag (confirmed in hardware)

		RealVDP = GetRealVDP();
        // TODO heat map debug
		//UpdateHeatVDP(RealVDP);
		VDP[RealVDP]=data;
		VDPMemInited[RealVDP]=1;

		// before the breakpoint, check and emit debug if we messed up the disk buffers
		if (getInterestingData(DATA_SYSTEM_TYPE) == SYSTEM_TI99) {
			int nTop = getInterestingData(DATA_TMS9900_8370);
			if (nTop <= 0x3be3) {
				// room for at least one disk buffer
				bool bFlag = false;

				if ((RealVDP == nTop+1) && (data != 0xaa)) {	bFlag = true;	}
				if ((RealVDP == nTop+2) && (data != 0x3f)) {	bFlag = true;	}
				if ((RealVDP == nTop+4) && (data != 0x11)) {	bFlag = true;	}
				if ((RealVDP == nTop+5) && (data > 9))		{	bFlag = true;	}

				if (bFlag) {
					debug_write("VDP disk buffer header corrupted at PC >%04X", getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC));
				}
			}
		}

		// check breakpoints against what was written to where - still assume internal address
        if (rmw != ACCESS_FREE) {
            testBreakpoint(false, VDPADD, isIO, data);
        }

        // verified on hardware
		vdpprefetch=data;
		vdpprefetchuninited = true;		// is it? you are reading back what you wrote. Probably not deliberate

		increment_vdpadd();
		redraw_needed=REDRAW_LINES;
	}
}

// claim resources from the core system, int is index from the host system
bool TMS9918::init(int index) {
    setIndex("TMS9918", index);

	if (nullptr == theCore->getTV()) {
		debug_write("No TV - improper system initialization.");
		return false;
	}

	// we could get faster performance if we allocated textures for
	// the character set and sprites, and blit them all at once, but that would
	// not allow for a scanline-based display... so we'll just pixel them...
	pDisplay = theCore->getTV()->requestLayer(TMS_WIDTH, TMS_HEIGHT);
	hzRate = 60;
	bDisableBlank = false;
	bDisableSprite = false;
	bDisableBackground = false;
    bDisableColorLayer = false;
	bDisablePatternLayer = false;

	bShowFPS = 1;

	vdpReset(true);     // todo: need to allow warm reset

    return true;
}

// process until the timestamp, in microseconds, is reached. The offset is arbitrary, on first run just accept it and return, likewise if it goes negative
bool TMS9918::operate(double timestamp) {
	if ((lastTimestamp == 0)||(timestamp < lastTimestamp)) {
		lastTimestamp = timestamp;
		return true;
	}

	// 262 scanlines at 60hz or 50hz, calculate a time per scanline in microseconds
	// the fraction is necessary for accurate timing
	// 262 lines, 342 pixel clocks per line
	const double timePerScanline = 1000000.0 / (double)(hzRate*262);

	// TODO: debug needs a way to force a full frame update
	// can now do this with TV->setDrawReady(true), frames are now always being drawn, not tied to CPU...

	while (lastTimestamp + timePerScanline < timestamp) {
		++vdpscanline;
		if (vdpscanline == TMS_DISPLAY_HEIGHT+TMS_FIRST_DISPLAY_LINE) {
			// set the vertical interrupt
			VDPS|=VDPS_INT;
		} else if (vdpscanline == TMS_HEIGHT) {
			// tell the TV to draw the frame
			theCore->getTV()->setDrawReady(true);
		} else if (vdpscanline >= TMS_HEIGHT+TMS_BLANKING) {
			// reset the raster
			vdpscanline = 0;
		}

#if 0
		// update the GPU
		// first GPU scanline is first line of active display
		// the blanking is not quite right. We expect the scanline
		// to be set before the line is buffered, and blank to
		// be set after it's cached. Since the GPU doesn't really
		// interleave, we always set blanking true and play with
		// the scanline so it works. TODO: fix that
		int gpuScanline = vdpscanline - 27;		// this value is correct for scanline pics

		if (gpuScanline < 0) gpuScanline+=262;
		if ((gpuScanline > 255)||(gpuScanline < 0)) {
			VDP[0x7000]=255;
		} else {
			VDP[0x7000]=gpuScanline;
		}
		VDP[0x7001] = 0x01;		// hblank OR vblank
#endif

		// process this scanline
		VDPdisplay(vdpscanline);

		// update the timestamp
		lastTimestamp += timePerScanline;
	}

	return true;
}

// release everything claimed in init, save NV data, etc
bool TMS9918::cleanup() {
	pDisplay = nullptr;
	return true;
}

// dimensions of a text mode output screen - either being 0 means none
void TMS9918::getDebugSize(int &x, int &y) {
	x=44; y=11;
}

// output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
void TMS9918::getDebugWindow(char *buffer) {
	int tmp1,tmp2;

	buffer += sprintf(buffer, "VDP0   %02X   ", VDPREG[0]);
	if (VDPREG[0] & 0xfc) buffer += sprintf(buffer, "??? "); else buffer += sprintf(buffer, "    ");
	if (VDPREG[0] & 0x02) buffer += sprintf(buffer, "BMP "); else buffer += sprintf(buffer, "    ");
	if (VDPREG[0] & 0x01) buffer += sprintf(buffer, "EXT "); else buffer += sprintf(buffer, "    ");
	buffer += sprintf(buffer, "\r\n");

	buffer += sprintf(buffer, "VDP1   %02X   ", VDPREG[1]);
	if (VDPREG[1] & 0x80) buffer += sprintf(buffer, "16k "); else buffer += sprintf(buffer, "    ");
	if (VDPREG[1] & 0x40) buffer += sprintf(buffer, "EN "); else buffer += sprintf(buffer, "   ");
	if (VDPREG[1] & 0x20) buffer += sprintf(buffer, "EI "); else buffer += sprintf(buffer, "   ");
	if (VDPREG[1] & 0x10) buffer += sprintf(buffer, "TXT "); else buffer += sprintf(buffer, "    ");
	if (VDPREG[1] & 0x08) buffer += sprintf(buffer, "MUL "); else buffer += sprintf(buffer, "    ");
	if (VDPREG[1] & 0x04) buffer += sprintf(buffer, "??? "); else buffer += sprintf(buffer, "    ");
	if (VDPREG[1] & 0x02) buffer += sprintf(buffer, "DBL "); else buffer += sprintf(buffer, "    ");
	if (VDPREG[1] & 0x01) buffer += sprintf(buffer, "MAG "); else buffer += sprintf(buffer, "    ");
	buffer += sprintf(buffer, "\r\n");

	buffer += sprintf(buffer, "VDP2   %02X   SIT %04X", VDPREG[2], (VDPREG[2]&0x0f)*0x400);
	if (VDPREG[2]&0xf0) buffer+=sprintf(buffer, "???");
	buffer+= sprintf(buffer, "\r\n");

	buffer += sprintf(buffer, "VDP3   %02X   CT   %04X Mask %04X ", VDPREG[3], CT, CTsize);
	if (VDPREG[0]&0x02) {
		// bitmap - check if the mask has holes in it
		tmp1 = 16;
		tmp2 = CTsize;
		while ((tmp1--) && ((tmp2&0x8000)==0)) { tmp2<<=1; }
		if (tmp1 > 0) {
			while (tmp1--) {
				tmp2<<=1;
				if ((tmp2&0x8000)==0) {
					buffer+=sprintf(buffer, "???");
					break;
				}
			}
		}
	}
	buffer+= sprintf(buffer, "\r\n");

	buffer += sprintf(buffer, "VDP4   %02X   PDT  %02X Mask %04X", VDPREG[4], PDT, PDTsize);
	if (VDPREG[0]&0x02) {
		// bitmap - check if the mask has holes in it
		tmp1 = 16;
		tmp2 = PDTsize;
		while ((tmp1--) && ((tmp2&0x8000)==0)) { tmp2<<=1; }
		if (tmp1 > 0) {
			while (tmp1--) {
				tmp2<<=1;
				if ((tmp2&0x8000)==0) {
					buffer+=sprintf(buffer, "???");
					break;
				}
			}
		}
	} else {
		// non-bitmap, just check unused bits
		if (VDPREG[4]&0xf8) buffer+=sprintf(buffer, "???");
	}
	buffer+= sprintf(buffer, "\r\n");

	buffer += sprintf(buffer, "VDP5   %02X   SAL %04X", VDPREG[5], SAL);
	if (VDPREG[5]&0x80) buffer+=sprintf(buffer, "???");
	buffer+= sprintf(buffer, "\r\n");

	buffer += sprintf(buffer, "VDP6   %02X   SDT %04X", VDPREG[6], SDT);
	if (VDPREG[6]&0xf8) buffer+=sprintf(buffer, "???");
	buffer+= sprintf(buffer, "\r\n");

	buffer += sprintf(buffer, "VDP7   %02X   ", VDPREG[7]);
	if (VDPREG[0]&0x10) {
		// text mode, foreground matters
		buffer += sprintf(buffer, "%s on ", COLORNAMES[(VDPREG[7]*0xf0)>>4]);
	} else if (VDPREG[7]&0xf0) {
		buffer += sprintf(buffer, "??? on ");	// not that this is terribly critical, but technically wrong
	}
	buffer += sprintf(buffer, "%s\r\n\r\n", COLORNAMES[VDPREG[7]&0x0f]);

	buffer += sprintf(buffer, "VDPST: %02X   ", VDPS);
	if (VDPS & 0x80) buffer += sprintf(buffer, "VBL "); else buffer += sprintf(buffer, "    ");
	if (VDPS & 0x40) buffer += sprintf(buffer, "5SP "); else buffer += sprintf(buffer, "    ");
	if (VDPS & 0x20) buffer += sprintf(buffer, "COL "); else buffer += sprintf(buffer, "    ");
	buffer += sprintf(buffer, "5thSP: %02X\r\n\r\n", VDPS&0x1f);

	buffer += sprintf(buffer, "VDPAD: %04X\r\n", VDPADD);
}

// number of bytes needed to save state
// TODO: VDP save state could be tricky, let's make it work first
int TMS9918::saveStateSize() {
	return 0;
}

// write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
bool TMS9918::saveState(unsigned char *buffer) {
	return false;
}

// restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize
bool TMS9918::restoreState(unsigned char *buffer) {
	return false;
}

// Increment VDP Address
void TMS9918::increment_vdpadd() 
{
	VDPADD=(VDPADD+1) & 0x3fff;
}

// Return the actual 16k address taking the 4k mode bit
// into account.
int TMS9918::GetRealVDP() {
	int RealVDP;

	// TODO: remove the non-9918 stuff from this function...

	// The 9938 and 9958 don't honor this bit, they always assume 128k (which we don't emulate, but we can at least do 16k like the F18A)
	// note that the 128k hack actually does support 128k now... as needed. So if bEnable128k is on, we take VDPREG[14]&0x07 for the next 3 bits.
	if ((bEnable80Columns) || (VDPREG[1]&0x80)) {
		// 16k mode - address is 7 bits + 7 bits, so use it raw
		// xx65 4321 0654 3210
		// This mask is not really needed because VDPADD already tracks only 14 bits
		RealVDP = VDPADD;	// & 0x3FFF;

		if (bEnable128k) {
			RealVDP|=VDPREG[14]<<14;
		}
		
	} else {
		// 4k mode -- address is 6 bits + 6 bits, but because of the 16k RAMs,
		// it gets padded back up to 7 bit each for row and col
		// The actual method used is a little complex to describe (although
		// I'm sure it's simple in silicon). The lower 6 bits are used as-is.
		// The next /7/ bits are rotated left one position.. not really sure
		// why they didn't just do a 6 bit shift and lose the top bit, but
		// this does seem to match every test I throw at it now. Finally, the
		// 13th bit (MSB for the VDP) is left untouched. There are no fixed bits.
		// Test values confirmed on real console:
		// 1100 -> 0240
		// 1810 -> 1050
		// 2210 -> 2410
		// 2211 -> 2411
		// 2240 -> 2280
		// 3210 -> 2450
		// 3810 -> 3050
		// Of course, only after working all this out did I look at Sean Young's
		// document, which describes this same thing from the hardware side. Those
		// notes confirm mine.
		//
		//         static bits       shifted bits           rotated bit
		RealVDP = (VDPADD&0x203f) | ((VDPADD&0x0fc0)<<1) | ((VDPADD&0x1000)>>7);
	}

	// force 8k DRAMs (strip top row bit - this should be right - console doesn't work though)
	// That's okay, the VDP datasheet doesn't recognize 8k DRAM, it's just a note from the TI ROMs
	//	RealVDP&=0x1FFF;

	return RealVDP;
}

// Write to VDP Register
void TMS9918::wVDPreg(uint8_t r, uint8_t v) { 
	int t;

	if (r > 58) {
		debug_write("Writing VDP register more than 58 (>%02X) ignored...", r);
		return;
	}

	VDPREG[r]=v;

	// check breakpoints against what was written to where
	// registers are stored with an address starting at 0x8000 (0x8100, 0x8200, same as written)
	testBreakpoint(false, 0x8000+(r<<8), false, v);		// we don't know if it's I/O, but doesn't matter internally

	if (r==7)
	{	/* color of screen, set TV background color to match */
		t=v&0xf;
		ALLEGRO_COLOR bg;
		// pixel format ARGB
		bg.r = ((F18APalette[t]>>16)&0xff)/(double)255.0;
		bg.g = ((F18APalette[t]>>8)&0xff)/(double)255.0;
		bg.b = ((F18APalette[t])&0xff)/(double)255.0;
		bg.a = 1.0;
		theCore->getTV()->setBgColor(bg);
		redraw_needed=REDRAW_LINES;
	}

	if (!bEnable80Columns) {
		// TODO: above test not needed in 9918 chip
		// 
		// warn if setting 4k mode - the console ROMs actually do this often! However,
		// this bit is not honored on the 9938 and later, so is usually set to 0 there
		if ((r == 1) && ((v&0x80) == 0)) {
			// ignore if it's a console ROM access - it does this to size VRAM
			// todo: need to check using the console ROM map in InterestingData
			if (getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC) > 0x2000) {
				debug_write("WARNING: Setting VDP 4k mode at PC >%04X", getInterestingDataIndirect(INDIRECT_MAIN_CPU_PC));
			}
		}
	}

	// for the F18A GPU, copy it to RAM
	// TODO: not needed in 9918
//	VDP[0x6000+r]=v;
}

// Get table addresses from Registers
// We return reg0 since we do the bitmap filter here now
// TODO: no layer2 in 9918
// TODO: layer2 should have its own TV layer in F18A, that makes compositing easier
int TMS9918::gettables(int isLayer2)
{
	int reg0 = VDPREG[0];

	// TODO: need to enable this for the raw 9918 and remove from the rest
	// disable bitmap for 9918 (no A)
	//reg0&=~0x02;

	// TODO: remove test from 9918
	//if (!bEnable80Columns) {
		// disable 80 columns if not enabled
	//	reg0&=~0x04;
	//}

	/* Screen Image Table */
	// TODO: 80 column test
	if (/*(bEnable80Columns) &&*/ (reg0 & 0x04)) {
		// in 80-column text mode, the two LSB are some kind of mask that we here ignore - the rest of the register is larger
		// The 9938 requires that those bits be set to 11, therefore, the F18A treats 11 and 00 both as 00, but treats
		// 01 and 10 as their actual values. (Okay, that is a bit weird.) That said, the F18A still only honours the least
		// significant 4 bits and ignores the rest (the 9938 reads 7 bits instead of 4, masking as above).
		// So anyway, the goal is F18A support, but the 9938 mask would be 0x7C instead of 0x0C, and the shift was only 8?
		// TODO: check the 9938 datasheet - did Matthew get it THAT wrong? Or does the math work out anyway?
		// Anyway, this works for table at >0000, which is most of them.
		SIT=(VDPREG[2]&0x0F);
		if ((SIT&0x03)==0x03) SIT&=0x0C;	// mask off a 0x03 pattern, 0x00,0x01,0x02 left alone
		SIT<<=10;
	} else {
		SIT=((VDPREG[2]&0x0f)<<10);
	}
	/* Sprite Attribute List */
	SAL=((VDPREG[5]&0x7f)<<7);
	/* Sprite Descriptor Table */
	SDT=((VDPREG[6]&0x07)<<11);

	// The normal math for table addresses isn't quite right in bitmap mode
	// The PDT and CT have different math and a size setting
	if (reg0&0x02) {
		// this is for bitmap modes
		CT=(VDPREG[3]&0x80) ? 0x2000 : 0;
		CTsize=((VDPREG[3]&0x7f)<<6)|0x3f;
		PDT=(VDPREG[4]&0x04) ? 0x2000 : 0;
		PDTsize=((VDPREG[4]&0x03)<<11);
		if (VDPREG[1]&0x10) {	// in Bitmap text, we fill bits with 1, as there is no color table
			PDTsize|=0x7ff;
		} else {
			PDTsize|=(CTsize&0x7ff);	// In other bitmap modes we get bits from the color table mask
		}
	} else {
		// this is for non-bitmap modes
		/* Colour Table */
		CT=VDPREG[3]<<6;
		/* Pattern Descriptor Table */
		PDT=((VDPREG[4]&0x07)<<11);
		CTsize=32;
		PDTsize=2048;

        if (isLayer2) {
            // get the F18A layer information
            // TODO: bigger tables are possible with F18A
		    /* Colour Table */
		    CT=VDPREG[11]<<6;
		    CTsize=32;
            /* Screen Image Table */
		    SIT=((VDPREG[10]&0x0f)<<10);

            /* Pattern Descriptor Table */
            // TODO: Matt says there's a second pattern table, but I don't see it in the docs.
		    //PDT=((VDPREG[4]&0x07)<<11);
		    //PDTsize=2048;
        }
	}

    return reg0;
}

// rests the uninitalized memory tracking on demand
void TMS9918::resetMemoryTracking() {
	vdpprefetchuninited = true;
	memset(VDPMemInited, 0, sizeof(VDPMemInited));
}

// called from tiemul
void TMS9918::vdpReset(bool isCold) {
    // on cold reset, reload everything. 
    if (isCold) {
	    // todo: move the other system-level init (what does the VDP do?) into here
	    // convert from 12-bit to float and load F18APalette
		// ARGB palette
	    for (int idx=0; idx<64; idx++) {
		    int r = (F18APaletteReset[idx]&0xf00)>>8;
		    int g = (F18APaletteReset[idx]&0xf0)>>4;
		    int b = (F18APaletteReset[idx]&0xf);
		    F18APalette[idx] = (r<<20)|(r<<16)|(g<<12)|(g<<8)|(b<<4)|(b)|0xff000000;	// double up each palette gun, suggestion by Sometimes99er
	    }

		memset(VDPREG, 0, sizeof(VDPREG));
        memset(VDP, 0, sizeof(VDP));
    }
    //bF18AActive = false;

	vdpaccess=0;		// No VDP address writes yet 
	vdpwroteaddress=0;
	vdpscanline=0;
	vdpprefetch=0;		// Not really accurate, but eh
	resetMemoryTracking();
    redraw_needed = REDRAW_LINES;
}

// used by the GetChar and capture functions
// returns 32, 40 or 80, or -1 if invalid
int TMS9918::getCharsPerLine() {
	int nCharsPerLine=32;	// default for graphics mode

	int reg0 = gettables(0);

	if (!(VDPREG[1] & 0x40))		// Disable display
	{
		return -1;
	}

	if ((VDPREG[1] & 0x18)==0x18)	// MODE BITS 2 and 1
	{
		return -1;
	}

	if (VDPREG[1] & 0x10)			// MODE BIT 2
	{
		if (reg0 & 0x02) {			// BITMAP MODE BIT
			return -1;
		}
		
		nCharsPerLine=40;
		if (reg0&0x04) {
			// 80 column text (512+16 pixels across instead of 256+16)
			nCharsPerLine = 80;
		}
	}

	if (VDPREG[1] & 0x08)			// MODE BIT 1
	{
		return -1;
	}

	if (reg0 & 0x02) {			// BITMAP MODE BIT
		return -1;
	}

    return nCharsPerLine;
}


// Passed Windows mouse based X and Y, figure out the char under
// the pointer. If it's not a text mode or it's not printable, then
// return -1. Due to screen borders, we have a larger area than
// the TI actually displays.
// TODO: how to get double click - does that go into the TV interface? How does it get back to the keyboard?
char TMS9918::VDPGetChar(int x, int y, int width, int height) {
	// TODO: the screen sizes changed - these numbers might now be incorrect
	double nCharWidth=34.0;	// default for graphics mode (32 chars plus 2 chars of border)
	int ch;
	int nCharsPerLine=getCharsPerLine();

    if (nCharsPerLine == -1) {
        return -1;
    }

    // update chars width (number of chars across entire screen)
    if (nCharsPerLine == 40) {
		nCharWidth=45.3;			// text mode (40 chars plus 5 chars of border)
    } else if (nCharsPerLine == 80) {
    	nCharWidth = 88.0;
    }

	// If it wasn't text and we got here, then it's multicolor
	// nCharWidth now has the number of columns across. There are
	// always 24+2 rows.
	double nXWidth, nYHeight;

	nYHeight=height/26.0;
	nXWidth=width/nCharWidth;

	if ((nXWidth<1.0) || (nYHeight<1.0)) {
		// screen is too small to differentiate (this is unlikely)
		return -1;
	}

	int row=(int)(y/nYHeight)-1;
	if ((row < 0) || (row > 23)) {
		return -1;
	}

	int col;
	if (nCharWidth > 39.0) {
		// text modes
		col=(int)((x/nXWidth)-2.6);
		if ((col < 0) || (col >= nCharsPerLine)) {
			return -1;
		}
	} else {
		col=(int)(x/nXWidth)-1;
		if ((col < 0) || (col > 31)) {
			return -1;
		}
	}

	ch=VDP[SIT+(row*nCharsPerLine)+col];

	if (isprint(ch)) {
		return ch;
	}

	// this is really hacky but it should work ;)
	// handle the TI BASIC character offset
	if ((ch >= 0x80) && (isprint(ch-0x60))) {
		return ch-0x60;
	}

	return -1;
}

// Perform drawing of a single line
// Determines which screen mode to draw
void TMS9918::VDPdisplay(int scanline)
{
	int nMax;

	// if no display, can not draw
	if (NULL == pDisplay) return;
	if (NULL == pDisplay->bmp) return;

	int reg0 = gettables(0);
	int gfxline = scanline - TMS_FIRST_DISPLAY_LINE;
	if ((gfxline < 0) || (gfxline >= TMS_DISPLAY_HEIGHT)) {
		return;
	}

	// the mode is a 32-bit format with alpha, but I guess it might vary
	// format should be ARGB
	// Do not early RETURN after this lock!
	ALLEGRO_LOCKED_REGION *pImg = al_lock_bitmap_region(pDisplay->bmp, 0, scanline, TMS_WIDTH, 1, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
	if (NULL == pImg) {
		debug_write("Could not lock bitmap!");
		return;
	}
	pLine = NULL;	// make sure it's zeroed unless we need it

	// count down scanlines to redraw
	// todo: the redraw_needed code all seems pretty broken
	if (redraw_needed > 0) {
		--redraw_needed;

		// draw blanking area
		// we don't need to worry about the actual color, we just set hot pink transparent (R+B)
		// it's the alpha that matters ;)
#if 0
		if ((reg0&0x04)&&(VDPREG[1]&0x10)&&(bEnable80Columns)) {
			// 80 column text
			nMax = (512+16)/4;
		} else 
#endif
		{
			// all other modes
			nMax = TMS_WIDTH/4;	// because we plot 4 pixels per loop
		}

		pLine = ((uint32_t*)pImg->data);

		// blank out the entire line first (unrolled 4 times)
		uint32_t *plong = pLine;
		for (int idx=0; idx<nMax; ++idx) {
			*(plong++) = HOT_PINK_TRANS;	// hot pink transparent
			*(plong++) = HOT_PINK_TRANS;	// hot pink transparent
			*(plong++) = HOT_PINK_TRANS;	// hot pink transparent
			*(plong++) = HOT_PINK_TRANS;	// hot pink transparent
		}

		// now center the actual draw area
		pLine += TMS_FIRST_DISPLAY_PIXEL;

		// now that the blank is done, draw the display data
		if (!bDisableBlank) {
			if (!(VDPREG[1] & 0x40)) {	// Disable display
				// This case is hit if nothing else is being drawn (ie: disable background is set), otherwise the graphics modes call DrawSprites
				// as long as mode bit 2 is not set, sprites are okay
				if (/*(bF18AActive) ||*/ ((VDPREG[1] & 0x10) == 0)) {
					DrawSprites(gfxline);
				}
				goto bottom;
			}
		} 
		
		if ((!bDisableBackground) && (gfxline < 192) && (gfxline >= 0)) {
			for (int isLayer2=0; isLayer2<2; ++isLayer2) {
				reg0 = gettables(isLayer2);

				if ((VDPREG[1] & 0x18)==0x18)				// MODE BITS 2 and 1
				{
					VDPillegal(gfxline, isLayer2);
				} else if (VDPREG[1] & 0x10)				// MODE BIT 2
				{
					if (reg0 & 0x02) {						// BITMAP MODE BIT
						VDPtextII(gfxline, isLayer2);			// undocumented bitmap text mode
					} else if (reg0 & 0x04) {				// MODE BIT 4 (9938)
						VDPtext80(gfxline, isLayer2);			// 80-column text, similar to 9938/F18A
					} else {
						VDPtext(gfxline, isLayer2);				// regular 40-column text
					}
				} else if (VDPREG[1] & 0x08)				// MODE BIT 1
				{
					if (reg0 & 0x02) {						// BITMAP MODE BIT
						VDPmulticolorII(gfxline, isLayer2);		// undocumented bitmap multicolor mode
					} else {
						VDPmulticolor(gfxline, isLayer2);
					}
				} else if (reg0 & 0x02) {					// BITMAP MODE BIT
					VDPgraphicsII(gfxline, isLayer2);			// documented bitmap graphics mode
				} else {
					VDPgraphics(gfxline, isLayer2);
				}

#if 0
				// Tile layer 2, if applicable
				// TODO: sprite priority is not taken into account
				if ((!bF18AActive) || ((VDPREG[49]&0x80)==0)) {
					break;
				}
#else
				// never draw layer 2
				break;
#endif
			}
		} else {
			// This case is hit if nothing else is being drawn (ie: disable background is set), otherwise the graphics modes call DrawSprites
			// as long as mode bit 2 is not set, sprites are okay
			if (/*(bF18AActive) ||*/ ((VDPREG[1] & 0x10) == 0)) {
				DrawSprites(gfxline);
			}
		}
	} else {
		// we have to redraw the sprites even if the screen didn't change, so that collisions are updated
		// as the CPU may have cleared the collision bit
		// as long as mode bit 2 (text) is not set, and the display is enabled, sprites are okay
		if (/*(bF18AActive) ||*/ ((VDPREG[1] & 0x10) == 0)) {
			if ((bDisableBlank) || (VDPREG[1] & 0x40)) {
				DrawSprites(gfxline);
			}
		}
	}

bottom:
	al_unlock_bitmap(pDisplay->bmp);

	// TODO: replace with actual bottom of display frame - varies on F18A
	if (gfxline == 191) {
		if (bShowFPS) {
			static int cnt = 0;
			static time_t lasttime = 0;
			static char buf[32] = "";
			++cnt;
			if (time(NULL) != lasttime) {
				sprintf(buf, "%d", cnt);
				//debug_write("%d fps", cnt);
				cnt = 0;
				time(&lasttime);
			}
			// draw digits
			pImg = al_lock_bitmap_region(pDisplay->bmp, 0, 187, 12, 5, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
			if (NULL == pImg) {
				debug_write("Could not lock bitmap for FPS!");
				bShowFPS = 0;
			} else {
				pLine = NULL;	// make sure it's zeroed unless we need it

				for (int i2=0; i2<5; i2++) {
					uint32_t *pDat = ((uint32_t*)pImg->data) + pImg->pitch*i2/4;
					for (int idx = 0; idx<(signed)strlen(buf); idx++) {
						int digit = buf[idx] - '0';
						pDat = drawTextLine(pDat, digpat[digit][i2]);
						*(pDat++)=0;
					}
				}

				al_unlock_bitmap(pDisplay->bmp);
			}
		}
	}
}

// Draw graphics mode
// Layer 2 for F18A second tile layer!
void TMS9918::VDPgraphics(int scanline, int isLayer2)
{
	int t,o;				// temp variables
	int i2;					// temp variables
	int p_add;
	uint32_t fgc, bgc, c;
	unsigned char ch=0xff;
//	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

	if (pLine == NULL) return;
	uint32_t *plong = pLine;	// get base line address

	o=(scanline/8)*32;			// offset in SIT

	//for (i1=0; i1<192; i1+=8)		// y loop
	{ 
		for (i2=0; i2<256; i2+=8)	// x loop
		{ 
			if (VDPDebug) {
				ch=o&0xff;              // debug character simply increments
                if (o&0x100) {
                    // sprites in the second block
                    p_add=SDT+(ch<<3)+i3;   // calculate pattern address
                    fgc = 15-(VDPREG[7]&0x0f);  // color the opposite of the screen color
                    bgc = 0;                // transparent background
                } else {
			        p_add=PDT+(ch<<3)+i3;   // calculate pattern address
			        c = ch>>3;              // divide by 8 for color table
			        fgc=VDP[CT+c];          // extract color
			        bgc=fgc&0x0f;           // extract background color
			        fgc>>=4;                // mask foreground color
                }
			} else {
				ch=VDP[SIT+o];          // look up character
			    p_add=PDT+(ch<<3)+i3;   // calculate pattern address
			    c = ch>>3;              // divide by 8 for color table
			    fgc=VDP[CT+c];          // extract color
			    bgc=fgc&0x0f;           // extract background color
			    fgc>>=4;                // mask foreground color
			}
			o++;

			//for (i3=0; i3<8; i3++)
			{	
				t=VDP[p_add];
	
	            if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                    // for layer 2, we have to drop transparent pixels,
                    // but I don't want to slow down the normal draw that much
			        //for (i3=0; i3<8; i3++)
			        {	
                        // fix it so that bgc is transparent, then we only draw on that test
                        if (fgc==0) {
                            t=~t;
                            fgc=bgc;
                            //bgc=0;  // not needed, not going to use it
                        }
                        if (fgc != 0) {     // skip if fgc is also transparent
							fgc = F18APalette[fgc];

				            if (t&80) *(plong++) = fgc; else ++plong;
				            if (t&40) *(plong++) = fgc; else ++plong;
				            if (t&20) *(plong++) = fgc; else ++plong;
				            if (t&10) *(plong++) = fgc; else ++plong;
				            if (t&8) *(plong++) = fgc; else ++plong;
				            if (t&4) *(plong++) = fgc; else ++plong;
				            if (t&2) *(plong++) = fgc; else ++plong;
				            if (t&1) *(plong++) = fgc; else ++plong;
                        } else {
							plong+=8;
						}
			        }
                } else {
					if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc=F18APalette[fgc];
					if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc=F18APalette[bgc];

					if (t&0x80) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x40) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x20) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x10) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x8) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x4) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x2) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x1) *(plong++) = fgc; else *(plong++) = bgc;
                }
			}
		}
	}

	DrawSprites(scanline);

}

// Draw bitmap graphics mode
void TMS9918::VDPgraphicsII(int scanline, int isLayer2)
{
	int t,o;				// temp variables
	int i2;					// temp variables
	int p_add, c_add;
	int fgc, bgc;
	int table, Poffset, Coffset;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

	if (pLine == NULL) return;
	uint32_t *plong = pLine;	// get base line address

    if (isLayer2) {
        // I don't think you can do bitmap layer 2??
        return;
    }

	o=(scanline/8)*32;			// offset in SIT
//	table=0; Poffset=0; Coffset=0;

//	for (i1=0; i1<192; i1+=8)		// y loop
	{ 
//		if ((i1==64)||(i1==128)) {
//			table++;
//			Poffset=table*0x800;
//			Coffset=table*0x800;
//		}
		table = i1/64;
		Poffset=table*0x800;
		Coffset=table*0x800;

		for (i2=0; i2<256; i2+=8)	// x loop
		{ 
			if (VDPDebug) {
				ch=o&0xff;
			} else {
				ch=VDP[SIT+o];
			}
			
    		p_add=PDT+(((ch<<3)+Poffset)&PDTsize)+i3;
	    	c_add=CT+(((ch<<3)+Coffset)&CTsize)+i3;
			o++;

//			for (i3=0; i3<8; i3++)
			{	
                if (bDisablePatternLayer) {
                    t = 0x00;   // show background colors
                } else {
    				t=VDP[p_add];
                }
                if (bDisableColorLayer) {
                    fgc=15; // white
                    bgc=1;  // black
                } else {
    				fgc=VDP[c_add];
	    			bgc=fgc&0x0f;
    				fgc>>=4;
                }
				{
					if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc=F18APalette[fgc];
					if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc=F18APalette[bgc];

					if (t&0x80) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x40) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x20) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x10) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x8) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x4) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x2) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x1) *(plong++) = fgc; else *(plong++) = bgc;
				}
			}
		}
	}

	DrawSprites(scanline);

}

// Draw text mode 40x24
void TMS9918::VDPtext(int scanline, int isLayer2)
{ 
	int t,o;
	int i2;
	int fgc, bgc, p_add;
	unsigned char ch=0xff;
//	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

	if (pLine == NULL) return;
	uint32_t *plong = pLine+(TMS_FIRST_DISPLAY_TEXT-TMS_FIRST_DISPLAY_PIXEL);	// get base line address

	o=(scanline/8)*40;			// offset in SIT

	t=VDPREG[7];
	bgc=t&0xf;
	fgc=t>>4;
    if (isLayer2) {
        bgc=0;
    }

//	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
		for (i2=8; i2<248; i2+=6)				// x loop
		{ 
			if (VDPDebug) {
				ch = o & 0xff;
			} else {
				ch=VDP[SIT+o];
			}

#if 0
			if ((bF18AActive) && (VDPREG[50]&0x02)) {
                // per-cell attributes, so update the colors
                if (isLayer2) {
                    t = VDP[VDPREG[11]*64 + o];
                    // BG is transparent (todo is that true?)
	                fgc=t>>4;
                } else {
                    t = VDP[VDPREG[3]*64 + o];
                }
	            bgc=t&0xf;
	            fgc=t>>4;
            }
#endif

			p_add=PDT+(ch<<3)+i3;
			o++;

            if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                // for layer 2, we have to drop transparent pixels,
                // but I don't want to slow down the normal draw that much
			    //for (i3=0; i3<8; i3++)
			    {	
				    t=VDP[p_add];
                    if (fgc != 0) {     // skip if fgc is also transparent
						fgc = F18APalette[fgc];

				        if (t&80) *(plong++) = fgc; else ++plong;
				        if (t&40) *(plong++) = fgc; else ++plong;
				        if (t&20) *(plong++) = fgc; else ++plong;
				        if (t&10) *(plong++) = fgc; else ++plong;
				        if (t&8) *(plong++) = fgc; else ++plong;
				        if (t&4) *(plong++) = fgc; else ++plong;
                    } else {
						plong+=6;
					}
			    }
            } else {
    //			for (i3=0; i3<8; i3++)		// 6 pixels wide
			    {	
				    t=VDP[p_add];
					if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc=F18APalette[fgc];
					if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc=F18APalette[bgc];

					if (t&0x80) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x40) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x20) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x10) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x8) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x4) *(plong++) = fgc; else *(plong++) = bgc;
			    }
            }
		}
	}

#if 0
    // no sprites in text mode, unless f18A unlocked
    if ((bF18AActive) && (!isLayer2)) {
        // todo: layer 2 has sprite dependency concerns
	    DrawSprites(scanline);
    }
#endif
}

// Draw bitmap text mode 40x24
void TMS9918::VDPtextII(int scanline, int isLayer2)
{ 
	int t,o;
	int i2;
	int fgc,bgc, p_add;
	int table, Poffset;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

	if (pLine == NULL) return;
	uint32_t *plong = pLine;	// get base line address

	o=(scanline/8)*40;			// offset in SIT

	t=VDPREG[7];
	bgc=t&0xf;
	fgc=t>>4;
    if (isLayer2) {
        bgc=0;
    }

	table=0; Poffset=0;

//	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
//		if ((i1==64)||(i1==128)) {
//		
	table++;
//			Poffset=table*0x800;
//		}
		table = i1/64;
		Poffset=table*0x800;

		for (i2=8; i2<248; i2+=6)				// x loop
		{ 
			if (VDPDebug) {
				ch=o&0xff;
			} else {
				ch=VDP[SIT+o];
			}

			p_add=PDT+(((ch<<3)+Poffset)&PDTsize)+i3;
			o++;

            if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                // for layer 2, we have to drop transparent pixels,
                // but I don't want to slow down the normal draw that much
			    //for (i3=0; i3<8; i3++)
			    {	
				    t=VDP[p_add];
                    if (fgc != 0) {     // skip if fgc is also transparent
						fgc = F18APalette[fgc];

				        if (t&80) *(plong++) = fgc; else ++plong;
				        if (t&40) *(plong++) = fgc; else ++plong;
				        if (t&20) *(plong++) = fgc; else ++plong;
				        if (t&10) *(plong++) = fgc; else ++plong;
				        if (t&8) *(plong++) = fgc; else ++plong;
				        if (t&4) *(plong++) = fgc; else ++plong;
                    } else {
						plong+=6;
					}
			    }
            } else {
    //			for (i3=0; i3<8; i3++)		// 6 pixels wide
			    {	
				    t=VDP[p_add];
					if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc=F18APalette[fgc];
					if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc=F18APalette[bgc];

					if (t&0x80) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x40) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x20) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x10) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x8) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x4) *(plong++) = fgc; else *(plong++) = bgc;
			    }
            }
		}
	}
	// no sprites in text mode
	// TODO: but F18A allows it - if F18A allows bitmap text mode...
}

// Draw text mode 80x24 (note: 80x26.5 mode not supported, blink not supported)
void TMS9918::VDPtext80(int scanline, int isLayer2)
{ 
	int t,o;
	int i2;
	int fgc,bgc, p_add;
	unsigned char ch=0xff;
//	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

	if (pLine == NULL) return;
	uint32_t *plong = pLine;	// get base line address

	o=(scanline/8)*80;				// offset in SIT

	t=VDPREG[7];
	bgc=t&0xf;
	fgc=t>>4;
    if (isLayer2) {
        bgc=0;
    }

//	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
		for (i2=8; i2<488; i2+=6)				// x loop
		{ 
			if (VDPDebug) {
				ch=o&0xff;
			} else {
				ch=VDP[SIT+o];
			}

#if 0
            if ((bF18AActive) && (VDPREG[50]&0x02)) {
                // per-cell attributes, so update the colors
                if (isLayer2) {
                    t = VDP[VDPREG[11]*64 + o];
                } else {
                    t = VDP[VDPREG[3]*64 + o];
                }
	            bgc=t&0xf;
	            fgc=t>>4;
            }
#endif

			p_add=PDT+(ch<<3)+i3;
			o++;

            if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                // for layer 2, we have to drop transparent pixels,
                // but I don't want to slow down the normal draw that much
			    //for (i3=0; i3<8; i3++)
			    {	
				    t=VDP[p_add];
                    if (fgc != 0) {     // skip if fgc is also transparent
						fgc = F18APalette[fgc];

				        if (t&80) *(plong++) = fgc; else ++plong;
				        if (t&40) *(plong++) = fgc; else ++plong;
				        if (t&20) *(plong++) = fgc; else ++plong;
				        if (t&10) *(plong++) = fgc; else ++plong;
				        if (t&8) *(plong++) = fgc; else ++plong;
				        if (t&4) *(plong++) = fgc; else ++plong;
                    } else {
						plong+=6;
					}
			    }
            } else {
                //			for (i3=0; i3<8; i3++)		// 6 pixels wide
			    {	
				    t=VDP[p_add];
					if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc=F18APalette[fgc];
					if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc=F18APalette[bgc];

					if (t&0x80) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x40) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x20) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x10) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x8) *(plong++) = fgc; else *(plong++) = bgc;
					if (t&0x4) *(plong++) = fgc; else *(plong++) = bgc;
			    }
            }
		}
	}

#if 0
    // no sprites in text mode, unless f18A unlocked
    // TODO: sprites don't render correctly in the wider 80 column mode...
	// TODO: this might be a good reason to render sprites on their own TV layer...
    if ((bF18AActive) && (!isLayer2)) {
        // todo: layer 2 has sprite dependency concerns
	    DrawSprites(scanline);
    }
#endif
}

// Draw Illegal mode (similar to text mode)
void TMS9918::VDPillegal(int scanline, int isLayer2)
{ 
	int t;
	int i2;
	int fgc,bgc;
//	const int i1 = scanline&0xf8;
//	const int i3 = scanline&0x07;
	(void)scanline;		// scanline is irrelevant

	if (pLine == NULL) return;
	uint32_t *plong = pLine;	// get base line address

	t=VDPREG[7];
	bgc=t&0xf;
	fgc=t>>4;

	// Each character is made up of rows of 4 pixels foreground, 2 pixels background
//	for (i1=0; i1<192; i1+=8)					// y loop
	{ 
		for (i2=8; i2<248; i2+=6)				// x loop
		{ 
            if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                // for layer 2, we have to drop transparent pixels,
                // but I don't want to slow down the normal draw that much
				// TODO: F18A probably doesn't support illegal mode... so would not draw this
			    //for (i3=0; i3<8; i3++)
			    {	
                    // fix it so that bgc is transparent, then we only draw on that test
                    if (fgc!=0) {
						fgc = F18APalette[fgc];

						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = fgc;
                    } else {
						plong+=4;
					}
                    if (bgc!=0) {
						bgc = F18APalette[bgc];

						*(plong++) = bgc;
						*(plong++) = bgc;
                    } else {
						plong += 2;
					}
			    }
            } else {
    //			for (i3=0; i3<8; i3++)				// 6 pixels wide
			    {	
					if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc = F18APalette[fgc];
					if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc = F18APalette[bgc];

					*(plong++) = fgc;
					*(plong++) = fgc;
					*(plong++) = fgc;
					*(plong++) = fgc;
					*(plong++) = bgc;
					*(plong++) = bgc;
				}
            }
		}
	}
	// no sprites in this mode
}

// Draw Multicolor Mode
void TMS9918::VDPmulticolor(int scanline, int isLayer2) 
{
	int o;				// temp variables
	int i2;				// temp variables
	int p_add;
	int fgc, bgc;
	int off;
	unsigned char ch=0xff;
//	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x04;
//	const int i4 = scanline&0x03;

	if (pLine == NULL) return;
	uint32_t *plong = pLine;	// get base line address

	o=(scanline/8)*32;			// offset in SIT
	off=(scanline>>2)&0x06;		// offset in pattern

//	for (i1=0; i1<192; i1+=8)									// y loop
	{ 
		for (i2=0; i2<256; i2+=8)								// x loop
		{ 
			if (VDPDebug) {
				ch=o&0xff;
			} else {
				ch=VDP[SIT+o];
			}

			p_add=PDT+(ch<<3)+off+(i3>>2);
			o++;

//			for (i3=0; i3<7; i3+=4)
			{	
				fgc=VDP[p_add];
				bgc=fgc&0x0f;
				fgc>>=4;
	
                if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                    // for layer 2, we have to drop transparent pixels,
                    // but I don't want to slow down the normal draw that much
			        //for (i3=0; i3<8; i3++)
			        {	
                        if (fgc != 0) {     // skip if fgc is also transparent
							fgc = F18APalette[fgc];

							*(plong++) = fgc;
							*(plong++) = fgc;
							*(plong++) = fgc;
							*(plong++) = fgc;
                        } else {
							plong += 4;
						}
                        if (bgc != 0) {
							bgc = F18APalette[bgc];

							*(plong++) = bgc;
							*(plong++) = bgc;
							*(plong++) = bgc;
							*(plong++) = bgc;
                        } else {
							plong += 4;
						}
			        }
                } else {
    //				for (i4=0; i4<4; i4++) 
				    {
						if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc = F18APalette[fgc];
						if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc = F18APalette[bgc];

						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = bgc;
						*(plong++) = bgc;
						*(plong++) = bgc;
						*(plong++) = bgc;
				    }
                }
			}
		}
//		off+=2;
//		if (off>7) off=0;
	}

	DrawSprites(scanline);

	return;
}

// Draw Bitmap Multicolor Mode
// TODO not proven to be correct anymore
void TMS9918::VDPmulticolorII(int scanline, int isLayer2) 
{
	int o;						// temp variables
	int i2;						// temp variables
	int p_add;
	int fgc, bgc;
//	int off;
	int table, Poffset;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x04;
//	const int i4 = scanline&0x03;

	if (pLine == NULL) return;
	uint32_t *plong = pLine;	// get base line address

	o=(scanline/8)*32;			// offset in SIT
//	off=(scanline>>2)&0x06;		// offset in pattern

	table=0; Poffset=0;

//	for (i1=0; i1<192; i1+=8)									// y loop
	{ 
//		if ((i1==64)||(i1==128)) {
//			table++;
//			Poffset=table*0x800;
//		}
		table = i1/64;
		Poffset=table*0x800;

		for (i2=0; i2<256; i2+=8)								// x loop
		{ 
			if (VDPDebug) {
				ch=o&0xff;
			} else {
				ch=VDP[SIT+o];
			}

			p_add=PDT+(((ch<<3)+Poffset)&PDTsize)+i3;
			o++;

//			for (i3=0; i3<7; i3+=4)
			{	
				fgc=VDP[p_add++];
				bgc=fgc&0x0f;
				fgc>>=4;
	
                if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                    // for layer 2, we have to drop transparent pixels,
                    // but I don't want to slow down the normal draw that much
			        //for (i3=0; i3<8; i3++)
			        {	
                        if (fgc != 0) {     // skip if fgc is also transparent
							fgc = F18APalette[fgc];

							*(plong++) = fgc;
							*(plong++) = fgc;
							*(plong++) = fgc;
							*(plong++) = fgc;
                        } else {
							plong += 4;
						}
                        if (bgc != 0) {
							bgc = F18APalette[bgc];

							*(plong++) = bgc;
							*(plong++) = bgc;
							*(plong++) = bgc;
							*(plong++) = bgc;
                        } else {
							plong += 4;
						}
			        }
                } else {
    //				for (i4=0; i4<4; i4++) 
				    {
						if (fgc == 0) fgc = HOT_PINK_TRANS; else fgc = F18APalette[fgc];
						if (bgc == 0) bgc = HOT_PINK_TRANS; else bgc = F18APalette[bgc];

						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = fgc;
						*(plong++) = bgc;
						*(plong++) = bgc;
						*(plong++) = bgc;
						*(plong++) = bgc;
				    }
                }
			}
		}
//		off+=2;
//		if (off>7) off=0;
	}

	DrawSprites(scanline);

	return;
}

// renders a string to the buffer - '1' is white, anything else black
// returns new pDat
unsigned int* TMS9918::drawTextLine(uint32_t *pDat, const char *buf) {
	if (pDat == NULL) return pDat;

	for (int idx = 0; idx<(signed)strlen(buf); idx++) {
        if (buf[idx] == '1') {
    		*(pDat++)=0xffffffff;
		} else {
			*(pDat++)=0;
		}
	}
    return pDat;
}

// Draw Sprites into the backbuffer
// TODO: put sprites on their own layer. This function
// can do the layer lock itself rather than use the
// member pLine
void TMS9918::DrawSprites(int scanline)
{
	// fifth sprite on a scanline, or last sprite processed in table
	// note that this value counts up as
	// it processes the sprite list (or at least,
	// it stores the last sprite processed).
	// Once latched, it does not change unless the status bit is cleared.
	// That behaviour (except the TODO) verified on hardware.
	// TODO:	does it count in real time? That is, if there are no overruns, is the value semi-random? Apparently yes!
	// TODO:	does it also latch when F (frame/vdp sync) is set? MAME does this. For now we latch only for 5S (I can't really do 'F' right now,
	//			it's always set when I get to this function!) Datasheet says yes though.
	// Ah ha: datasheet says:
	// "The 5S status flag in the status register is set to a 1 whenever there are five or more sprites on a horizontal
	// line (lines 0 to 192) and the frame flag is equal to a 0.  The 5S status flag is cleared to a 0 after the 
	// status register is read or the VDP is externally reset.  The number of the fifth sprite is placed into the 
	// lower 5 bits of the status register when the 5S flag is set and is valid whenever the 5S flag is 1.  The 
	// setting of the fifth sprite flag will not cause an interrupt"
	// Real life test data:
	// 4 sprites in sprite list on same line (D0 on sprite 4): A4 after a frame, 04 if read again immediately (F,C, last sprite 4)
	// 5 sprites: E4 after frame (F,5,C, 5th sprite 4), 05 if read again immediately (last sprite 5)
	// 6 sprites: E4 after frame (F,5,C, 5th sprite 4), 06 if read again immediately (last sprite 6)
	// 11 sprites on 2 lines (0-5,6-10): E4 (F,5,C, 5th sprite 4), 0B (last sprite 11)
	// 10 sprites on 2 lines (5-9,0-4): E9 (F,5,C, 5th sprite 9), 0A (last sprite 10)
	//
	// TODO: so why does Miner2049 fail during execution? And why did Rasmus see higher numbers than
	// he expected to see in Bouncy? Miner2049 does not check collisions unless it sees a status bit of >02,
	// which is a clear bug (they probably meant >20 - collision). This probably worked because there is a
	// pretty good likelihood of getting a count with that value in it.
	//
	// The real-world case I did not test: all 32-sprites are active, what is the 5th sprite index when
	// it is not latched by 5S, at the end of the frame. 0 works with Bouncy, but 1F works with Miner.
	// Theory: the correct value is probably 00 (>1F + 1 wrapped to 5 bits), and Miner relies on the
	// semi-random nature of reading the flag mid-frame instead of only at the end of a frame. We can't
	// reproduce this very well without a line-by-line VDP.
	//
	// TODO: fix Miner2049 without a hack.
	// TODO: and do all the above properly, now that we're scanline based - it'll probably be faster ;)
	int b5OnLine=-1;

	if (bDisableSprite) {
		return;
	}

	if (NULL == pLine) return;

	// check if b5OnLine is already latched, and set it if so.
	if (VDPS & VDPS_5SPR) {
		b5OnLine = VDPS & 0x1f;
	}

	// generate a list of sprites to process in reverse order
	int sprList[32];

	// set up the draw
	memset(SprColBuf, 0, 256);	// only need one line now
	SprColFlag=0;
	
	int highest=31;
#if 0
        if (bF18AActive) {
			// TODO: make sure reg 0x33 is initialized to 31
            highest = VDPREG[0x33]&0x1f;		// F18A - configurable value
        }
#endif

	int nextSprite = 0;
	int height = 8;
	if (VDPREG[1]&0x01) height *= 2;	// magnified
	if (VDPREG[1]&0x02)	height *= 2;	// double size

    int max = 5;                    // 9918A - fifth sprite is lost
#if 0
    if (bF18AActive) {
        max = VDPREG[0x1e]&0x1f;    // F18A - configurable value
        if (max == 0) max = 5;      // assume jumper set to 9918A mode
    }
#endif
	if (!bUse5SpriteLimit) {
		max = highest+1;
	}

	for (int i1=0; i1<=highest; ++i1) {
		// TODO: F18A can do double-size sprites per sprite!
		//int dblSize = F18AECModeSprite ? VDP[curSAL] & 0x10 : VDPREG[1] & 0x2;
		int adr = SAL+(i1<<2);
		int yy = VDP[adr];
		if (yy == 0xd0) break;
		++yy;
		if (yy>225) yy-=256;
		if ((scanline >= yy) && (scanline < yy+height)) {
			sprList[nextSprite++] = adr;
			if (nextSprite >= max) {
				if (b5OnLine == -1) b5OnLine=i1;
				break;
			}
		}
	}

	// now sprList has a list of sprite addresses to draw, and nextSprite
	// points to 1 past the end of that list, which we'll walk backwards
	// in order to draw in the correct order
	// TODO: verify sprite Y coordinate is correct, especially with magnified sprites
	while (--nextSprite >= 0) {
		int curSAL = sprList[nextSprite];
		int yy=VDP[curSAL++]+1;			// sprite Y, it's stupid, cause 255 is line 0 
		if (yy>225) yy-=256;			// fade in from top: TODO: is this right??
		int xx=VDP[curSAL++];			// sprite X 
		int pat=VDP[curSAL++];			// sprite pattern index
		int mag = VDPREG[1] & 0x01;
		int dblSize = F18AECModeSprite ? VDP[curSAL] & 0x10 : VDPREG[1] & 0x2;
		if (dblSize) {
			pat=pat&0xfc;				// if double-sized, it must be a multiple of 4
		}
		int col=VDP[curSAL]&0xf;		// sprite color
		if (col == 0) col = HOT_PINK_TRANS; else col=F18APalette[col];
	
		if (VDP[curSAL]&0x80) {			// early clock
			xx-=32;
		}

		// work out which line of the sprite to draw
		int spriteline = scanline-yy;
		if (mag) spriteline/=2;
		if (spriteline>7) {
			spriteline-=8;
			++pat;
		}

		// TODO: F18A ECM sprites are up to 8 colors so need a different fetch system
		// for now, just collect the first pattern to scan out
		int p_add = SDT+(pat<<3)+(spriteline%8);
		uint32_t *plong = pLine;
		plong += xx;	// warning: can be negative, and can be offscreen!
		for (int cnt = 0; cnt < (dblSize ? 2:1); ++cnt) {
			int mask = 0x80;
			int p = VDP[p_add];
			for (int idx = 0; idx<8; ++idx) {
				// TODO: maximum width check - it's smaller in text mode on F18A
				if ((xx >= 0) && (xx <= 255)) {
					if (mag) {
						if (p & mask) {
							// collision buffer
							SprColFlag |= SprColBuf[xx];
							SprColBuf[xx] = 1;
							if (xx < 255) {
								SprColFlag |= SprColBuf[xx+1];
								SprColBuf[xx+1] = 1;
							}

							// rendering
							if (col != HOT_PINK_TRANS) {
								*plong = col;
								if (xx < 255) {
									// skipping just this is okay, cause it means end of line anyway
									*(plong+1) = col;
								}
							}
						}
					} else {
						if (p & mask) {
							// collision buffer
							SprColFlag |= SprColBuf[xx];
							SprColBuf[xx] = 1;

							// rendering
							if (col != HOT_PINK_TRANS) {
								*plong = col;
							}
						}
					}
				}

				if (mag) {
					xx += 2;
					plong += 2;
				} else {
					++xx;
					++plong;
				}
				mask >>= 1;
			}
			// Second character, if in double-size sprite mode
			p_add += 16;
		}
	}

	// Set the VDP collision bit
	if (SprColFlag) {
		VDPS|=VDPS_SCOL;
	}
	if (b5OnLine != -1) {
		VDPS &= (VDPS_INT|VDPS_5SPR|VDPS_SCOL);
		VDPS |= (b5OnLine&(~(VDPS_INT|VDPS_5SPR|VDPS_SCOL))) | VDPS_5SPR;
	} else {
		VDPS&=(VDPS_INT|VDPS_5SPR|VDPS_SCOL);
		// TODO: so if all 32 sprites are in used, the 5-on-line value for not
		// having 5 on a line anywhere will be 0? (0x1f+1)=0x20 -> 0x20&0x1f = 0!
		// See notes above.
		// The correct behaviour is probably to count in realtime - that goes in with
		// the scanline code. -- Except I don't think that will work here because
		// the CPU doesn't execute in parallel with this processing... random number
		// might actually BE better...
		VDPS|=(highest+1)&(~(VDPS_INT|VDPS_5SPR|VDPS_SCOL));
	}
}

#if 0
////////////////////////////////////////////////////////////
// Pixel mask
// Function by Rasmus to get 8 color pixels for one sprite for F18A
////////////////////////////////////////////////////////////
int pixelMask(int addr, int F18ASpriteColorLine[])
{
	int t = VDP[addr];
	if (F18AECModeSprite > 0) {
		for (int pix = 0; pix < 8; pix++) {
			F18ASpriteColorLine[pix] = ((t >> (7 - pix)) & 0x01);
		}		
		if (F18AECModeSprite > 1) {
			int t1 = VDP[addr + 0x0800]; 
			for (int pix = 0; pix < 8; pix++) {
				F18ASpriteColorLine[pix] |= ((t1 >> (7 - pix)) & 0x01) << 1;
			}		
			t |= t1;
			if (F18AECModeSprite > 2) {
				int t2 = VDP[addr + 0x1000]; 
				for (int pix = 0; pix < 8; pix++) {
					F18ASpriteColorLine[pix] |= ((t2 >> (7 - pix)) & 0x01) << 2;
				}		
				t |= t2;
			}
		}
	}
	return t;
}
#endif

#if 0
// F18A Status registers
// This function must only be called when the register is not 0
// and F18A is active
unsigned char getF18AStatus() {
	F18AStatusRegisterNo &= 0x0f;

	switch (F18AStatusRegisterNo) {
	case 1:
		{
			// ID0 ID1 ID2 0 0 0 blanking HF
			// F18A ID is 0xE0
			unsigned char ret = 0xe0;
			// blanking bit
			if (VDP[0x7001]) {
				// TODO: horizontal blank is not supported since we do scanlines?
				ret |= 0x02;
			}
			// HF
			if ((VDPREG[19] != 0) && (vdpscanline == VDPREG[19])) {
				ret |= 0x01;
			}
			return ret;
		}

	case 2:
		{
			// GPUST GDATA0 GDATA1 GDATA2 GDATA3 GDATA4 GDATA5 GDATA6
			// TODO GDATA not supported (how /is/ it supported?)
			if (pGPU->GetIdle() == 0) {
				return 0x80;
			} else {
				return 0x00;
			}
		}

	case 3:
		{
			// current scanline, 0 when not active
			// TODO: is that 0 bit true??
			if ((vdpscanline >= 0) && (vdpscanline < 192+27+24)) {
				return 0;
			} else {
				return VDP[0x7000];
			}
		}
				
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		// TODO: 64 bit counter - if it's still valid (it's not?)
		return 0;

	case 12:
	case 13:
		// Unused
		return 0;

	case 14:
		// VMAJOR0 VMAJOR1 VMAJOR2 VMAJOR3 VMINOR0 VMINOR1 VMINOR2 VMINOR3
		// TODO: we kind of lie about this...
		return 0x18;

	case 15:
		// VDP register read value - updated any time a VRAM address is set
		// TODO: what does this mean? I think it's the value that the TI would get?
		// So what would happen if the TI set this one? ;)
		return 0;
	}

	debug_write("Warning - bad register value %d", F18AStatusRegisterNo);
	return 0;
}
#endif

#if 0
// captures a text or graphics mode screen (will do bitmap too, assuming graphics mode)
// I think this is for edit->copy
CString captureScreen(int offset) {
    CString csout;
    int stride = getCharsPerLine();
    if (stride == -1) {
        return "";
    }

    gettables(0);

    for (int row = 0; row<24; ++row) {
        for (int col=0; col < stride; ++col) {
            int index = SIT+row*stride+col;
            if (index >= sizeof(VDP)) { row=25; break; }   // check for unlikely overrun case
            int c = VDP[index] + offset; 
            if ((c>=' ')&&(c < 127)) csout+=(char)c; else csout+='.';
        }
        csout+="\r\n";
    }

    return csout;
}
#endif
