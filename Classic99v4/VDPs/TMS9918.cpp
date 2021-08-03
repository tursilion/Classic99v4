// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// TODO: delete the F18A stuff once we have that version of the file - saving it for now so it's easier to make that version work
// Basically, I want to get this VDP working more or less like the old Classic99 one did, then I can split it up into the 9918A
// and F18A versions/overrides and clean this one up as a base class
// TODO: VDP register write breakpoints?

#include "TMS9918.h"
#include "..\EmulatorSupport\peripheral.h"
#include "..\EmulatorSupport\debuglog.h"
#include "..\EmulatorSupport\System.h"
#include "..\EmulatorSupport\interestingData.h"

// 16-bit 0rrrrrgggggbbbbb values
//int TIPALETTE[16]={ 
//	0x0000, 0x0000, 0x1328, 0x2f6f, 0x295d, 0x3ddf, 0x6949, 0x23be,
//	0x7d4a, 0x7def, 0x6b0a, 0x7330, 0x12c7, 0x6577, 0x6739, 0x7fff,
//};
// 32-bit 0RGB colors
//unsigned int TIPALETTE[16] = {
//	0x00000000,0x00000000,0x0020C840,0x0058D878,0x005050E8,0x007870F8,0x00D05048,
//	0x0040E8F0,0x00F85050,0x00F87878,0x00D0C050,0x00E0C880,0x0020B038,0x00C858B8,
//	0x00C8C8C8,0x00F8F8F8
//};
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

const char *digpat[10][5] = {
	"111",
	"101",
	"101",
	"101",
	"111",

    "010",
	"110",
	"010",
	"010",
	"111",

    "111",
	"001",
	"111",
	"100",
	"111",

    "111",
	"001",
	"011",
	"001",
	"111",

    "101",
	"101",
	"111",
	"001",
	"001",

    "111",
	"100",
	"111",
	"001",
	"111",

    "111",
	"100",
	"111",
	"101",
	"111",

    "111",
	"001",
	"001",
	"010",
	"010",

    "111",
	"101",
	"111",
	"101",
	"111",

    "111",
	"101",
	"111",
	"001",
	"111"
};

// some local defines
#define REDRAW_LINES 262


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
            operate(lastTimestamp+cycles);
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
			theCore->triggerBreakPoint();
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
    // address 0 - read data
    // address 1 - read status
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
					if (getInterestingData(DATA_BREAK_ON_DISK_CORRUPT) == DATA_TRUE) theCore->triggerBreakPoint();
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
	const int timePerScanline = 1000000.0 / (double)(hzRate*262);

	// TODO: debug needs a way to force a full frame update

	while (lastTimestamp + timePerScanline < timestamp) {
		++vdpscanline;
		if (vdpscanline == 192+27) {
			// set the vertical interrupt
			VDPS|=VDPS_INT;
			end_of_frame = 1;
		} else if (vdpscanline > 261) {
			vdpscanline = 0;

			// TODO: full frame ready to draw - tell TV system
			//SetEvent(BlitEvent);
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

		// are we off the screen?
		if (vdpscanline < 192+27+24) {
			// nope, we can process this one
			VDPdisplay(vdpscanline);
		}

		lastTimestamp += timePerScanline;
	}
}

// release everything claimed in init, save NV data, etc
bool TMS9918::cleanup() {

}

// dimensions of a text mode output screen - either being 0 means none
void TMS9918::getDebugSize(int &x, int &y) {

}

// output the current debug information into the buffer, sized (x+2)*y to allow for windows style line endings
void TMS9918::getDebugWindow(char *buffer) {

}

// number of bytes needed to save state
int TMS9918::saveStateSize() {

}

// write state data into the provided buffer - guaranteed to be the size returned by saveStateSize
bool TMS9918::saveState(unsigned char *buffer) {

}

// restore state data from the provided buffer - guaranteed to be the size returned by saveStateSize
bool TMS9918::restoreState(unsigned char *buffer) {

}

// check VDP breakpoints
void TMS9918::testBreakpoint(bool isRead, int addr, bool isIO, int data) {

}

//////////////////////////////////////////////////////
// Increment VDP Address
//////////////////////////////////////////////////////
void TMS9918::increment_vdpadd() 
{
	VDPADD=(++VDPADD) & 0x3fff;
}

//////////////////////////////////////////////////////
// Return the actual 16k address taking the 4k mode bit
// into account.
//////////////////////////////////////////////////////
int TMS9918::GetRealVDP() {
	int RealVDP;

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

////////////////////////////////////////////////////////////////
// Write to VDP Register
////////////////////////////////////////////////////////////////
void TMS9918::wVDPreg(uint8_t r, uint8_t v) { 
	int t;

	if (r > 58) {
		debug_write("Writing VDP register more than 58 (>%02X) ignored...", r);
		return;
	}

	VDPREG[r]=v;

	// check breakpoints against what was written to where
	for (int idx=0; idx<nBreakPoints; idx++) {
		switch (BreakPoints[idx].Type) {
			case BREAK_EQUALS_VDPREG:
				if ((r == BreakPoints[idx].A) && ((v&BreakPoints[idx].Mask) == BreakPoints[idx].Data)) {
                    if ((!bIgnoreConsoleBreakpointHits) || (pCurrentCPU->GetPC() > 0x1fff)) {
    					TriggerBreakPoint();
                    }
				}
				break;
		}
	}

	if (r==7)
	{	/* color of screen, set color 0 (trans) to match */
		/* todo: does this save time in drawing the screen? it's dumb */
		t=v&0xf;
		if (t) {
			F18APalette[0]=F18APalette[t];
		} else {
			F18APalette[0]=0x000000;	// black
		}
		redraw_needed=REDRAW_LINES;
	}

	if (!bEnable80Columns) {
		// warn if setting 4k mode - the console ROMs actually do this often! However,
		// this bit is not honored on the 9938 and later, so is usually set to 0 there
		if ((r == 1) && ((v&0x80) == 0)) {
			// ignore if it's a console ROM access - it does this to size VRAM
			if (pCurrentCPU->GetPC() > 0x2000) {
				debug_write("WARNING: Setting VDP 4k mode at PC >%04X", pCurrentCPU->GetPC());
			}
		}
	}

	// for the F18A GPU, copy it to RAM
	VDP[0x6000+r]=v;
}

#if 0
// TODO: this is only for tiles, and only for ECM0
#define GETPALETTEVALUE(n) F18APalette[(bF18AActive?((VDPREG[0x18]&03)<<4)+(n) : (n))]
#endif

//////////////////////////////////////////////////////////
// Helpers for the TV controls
//////////////////////////////////////////////////////////
void GetTVValues(double *hue, double *sat, double *cont, double *bright, double *sharp) {
	*hue=tvSetup.hue;
	*sat=tvSetup.saturation;
	*cont=tvSetup.contrast;
	*bright=tvSetup.brightness;
	*sharp=tvSetup.sharpness;
}

void SetTVValues(double hue, double sat, double cont, double bright, double sharp) {
	tvSetup.hue=hue;
	tvSetup.saturation=sat;
	tvSetup.contrast=cont;
	tvSetup.brightness=bright;
	tvSetup.sharpness=sharp;
	if (sms_ntsc_init) {
		if (!TryEnterCriticalSection(&VideoCS)) {
			return;		// do it later
		}
		sms_ntsc_init(&tvFilter, &tvSetup);
		LeaveCriticalSection(&VideoCS);
	}
}

//////////////////////////////////////////////////////////
// Get table addresses from Registers
// We return reg0 since we do the bitmap filter here now
//////////////////////////////////////////////////////////
int gettables(int isLayer2)
{
	int reg0 = VDPREG[0];
	if (nSystem == 0) {
		// disable bitmap for 99/4
		reg0&=~0x02;
	}
	if (!bEnable80Columns) {
		// disable 80 columns if not enabled
		reg0&=~0x04;
	}

	/* Screen Image Table */
	if ((bEnable80Columns) && (reg0 & 0x04)) {
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
void resetMemoryTracking() {
	vdpprefetchuninited = true;
	memset(VDPMemInited, 0, sizeof(VDPMemInited));
}

// called from tiemul
void vdpReset(bool isCold) {
    // on cold reset, reload everything. 
#if 0
    if (isCold) {
	    // todo: move the other system-level init (what does the VDP do?) into here
	    memcpy(F18APalette, F18APaletteReset, sizeof(F18APalette));
	    // convert from 12-bit to 24-bit
	    for (int idx=0; idx<64; idx++) {
		    int r = (F18APalette[idx]&0xf00)>>8;
		    int g = (F18APalette[idx]&0xf0)>>4;
		    int b = (F18APalette[idx]&0xf);
		    F18APalette[idx] = (r<<20)|(r<<16)|(g<<12)|(g<<8)|(b<<4)|b;	// double up each palette gun, suggestion by Sometimes99er
	    }
    }
    bF18AActive = false;
#endif

   	vdpaccess=0;		// No VDP address writes yet 
	vdpwroteaddress=0;
	vdpscanline=0;
	vdpprefetch=0;		// Not really accurate, but eh
	resetMemoryTracking();
    redraw_needed = REDRAW_LINES;

    if (!isCold) {
        memset(VDPREG, 0, sizeof(VDPREG));
        memset(VDP, 0, sizeof(VDP));
    }
}

////////////////////////////////////////////////////////////
// Startup and run VDP graphics interface
////////////////////////////////////////////////////////////
void VDPmain()
{	
	DWORD ret;
	HDC myDC;

	Init_2xSaI(888);

	// load the Filter DLL
	TVFiltersAvailable=0;
	hFilterDLL=LoadLibrary("FilterDll.dll");
	if (NULL == hFilterDLL) {
		debug_write("Failed to load filter library.");
	} else {
		sms_ntsc_init=(void (*)(sms_ntsc_t*,sms_ntsc_setup_t const *))GetProcAddress(hFilterDLL, "sms_ntsc_init");
		sms_ntsc_blit=(void (*)(sms_ntsc_t const *, unsigned int const *, long, int, int, void *, long))GetProcAddress(hFilterDLL, "sms_ntsc_blit");
		sms_ntsc_scanlines=(void (*)(void *, int, int, int))GetProcAddress(hFilterDLL, "sms_ntsc_scanlines");
		if ((NULL == sms_ntsc_blit) || (NULL == sms_ntsc_init) || (NULL == sms_ntsc_scanlines)) {
			debug_write("Failed to find entry points in filter library.");
			FreeLibrary(hFilterDLL);
			sms_ntsc_blit=NULL;
			sms_ntsc_init=NULL;
			sms_ntsc_scanlines=NULL;
			hFilterDLL=NULL;
		} else {
			// Some of these are set up by SetTVValues()
//			tvSetup.hue=0;			// -1.0 to +1.0
//			tvSetup.saturation=0;	// -1.0 to +1.0
//			tvSetup.contrast=0;		// -1.0 to +1.0
//			tvSetup.brightness=0;	// -1.0 to +1.0
//			tvSetup.sharpness=0;	// -1.0 to +1.0
			tvSetup.gamma=0;		// -1.0 to +1.0
			tvSetup.resolution=0;	// -1.0 to +1.0
			tvSetup.artifacts=0;	// -1.0 to +1.0
			tvSetup.fringing=0;		// -1.0 to +1.0
			tvSetup.bleed=0;		// -1.0 to +1.0
			tvSetup.decoder_matrix=NULL;
			tvSetup.palette_out=NULL;
			sms_ntsc_init(&tvFilter, &tvSetup);
			TVFiltersAvailable=1;
		}
	}

	hHQ4DLL=LoadLibrary("HQ4xDll.dll");
	if (NULL == hHQ4DLL) {
		debug_write("Failed to load HQ4 library.");
	} else {
		hq4x_init=(void (*)(void))GetProcAddress(hHQ4DLL, "hq4x_init");
		hq4x_process=(void (*)(unsigned char *pBufIn, unsigned char *pBufOut))GetProcAddress(hHQ4DLL, "hq4x_process");
		if ((NULL == hq4x_init) || (NULL == hq4x_process)) {
			debug_write("Failed to find entry points in HQ4x library.");
			FreeLibrary(hHQ4DLL);
			hq4x_init=NULL;
			hq4x_process=NULL;
			hHQ4DLL=NULL;
		} else {
			hq4x_init();
		}
	}

	myInfo.bmiHeader.biSize=sizeof(myInfo.bmiHeader);
	myInfo.bmiHeader.biWidth=256+16;
	myInfo.bmiHeader.biHeight=192+16;
	myInfo.bmiHeader.biPlanes=1;
	myInfo.bmiHeader.biBitCount=32;
	myInfo.bmiHeader.biCompression=BI_RGB;
	myInfo.bmiHeader.biSizeImage=0;
	myInfo.bmiHeader.biXPelsPerMeter=1;
	myInfo.bmiHeader.biYPelsPerMeter=1;
	myInfo.bmiHeader.biClrUsed=0;
	myInfo.bmiHeader.biClrImportant=0;

	memcpy(&myInfo2, &myInfo, sizeof(myInfo));
	myInfo2.bmiHeader.biWidth=512+32;
	myInfo2.bmiHeader.biHeight=384+29;

	memcpy(&myInfoTV, &myInfo2, sizeof(myInfo2));
	myInfoTV.bmiHeader.biWidth=TV_WIDTH;

	memcpy(&myInfo32, &myInfo, sizeof(myInfo));
	myInfo32.bmiHeader.biWidth*=4;
	myInfo32.bmiHeader.biHeight*=4;
	myInfo32.bmiHeader.biBitCount=32;

	memcpy(&myInfo80Col, &myInfo, sizeof(myInfo));
	myInfo80Col.bmiHeader.biWidth=512+16;

	myDC=GetDC(myWnd);
	tmpDC=CreateCompatibleDC(myDC);
	ReleaseDC(myWnd, myDC);

	SetupDirectDraw(0);

	// now we create a waitable object and sit on it - the main thread
	// will tell us when we should redraw the screen.
	BlitEvent=CreateEvent(NULL, false, false, NULL);
	if (NULL == BlitEvent)
		debug_write("Blit Event Creation failed");

	// layers all enabled at start
	bDisableBlank=false;
	bDisableSprite=false;
	bDisableBackground=false;

	debug_write("Starting video loop");
	redraw_needed=REDRAW_LINES;

	while (quitflag==0)
	{
		if ((ret=WaitForMultipleObjects(1, Video_hdl, false, 100)) != WAIT_TIMEOUT)
		{
			if (WAIT_FAILED==ret)
				ret=GetLastError();

			if (WAIT_OBJECT_0 == ret) {
				doBlit();
                continue;
			}

			// Don't ever spend all our time doing this!
			Sleep(5);		// rounds up to quantum
		}
	}

	if (NULL != hFilterDLL) {
		sms_ntsc_blit=NULL;
		sms_ntsc_init=NULL;
		sms_ntsc_scanlines=NULL;
		FreeLibrary(hFilterDLL);
		hFilterDLL=NULL;
	}
	takedownDirectDraw();
	DeleteDC(tmpDC);
	CloseHandle(BlitEvent);
}

// used by the GetChar and capture functions
// returns 32, 40 or 80, or -1 if invalid
int getCharsPerLine() {
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
char VDPGetChar(int x, int y, int width, int height) {
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

//////////////////////////////////////////////////////////
// Perform drawing of a single line
// Determines which screen mode to draw
//////////////////////////////////////////////////////////
void VDPdisplay(int scanline)
{
	int idx;
	DWORD longcol;
	DWORD *plong;
	int nMax;

	int reg0 = gettables(0);

	int gfxline = scanline - 27;	// skip top border

	if (redraw_needed) {
		// count down scanlines to redraw
		--redraw_needed;

		// draw blanking area
		if ((vdpscanline >= 0) && (vdpscanline < 192+27+24)) {
			// extra hack - we only have 8 pixels of border on each side, unlike the real VDP
			int tmplin = (199-(vdpscanline - 19-8));		// 27-8 = 19, don't remember where 199 comes from though...
			if ((tmplin >= 0) && (tmplin < 192+16)) {
				plong=(DWORD*)framedata;
				longcol=GETPALETTEVALUE(VDPREG[7]&0xf);
				if ((reg0&0x04)&&(VDPREG[1]&0x10)&&(bEnable80Columns)) {
					// 80 column text
					nMax = (512+16)/4;
				} else {
					// all other modes
					nMax = (256+16)/4;
				}
				plong += (tmplin*nMax*4);

				for (idx=0; idx<nMax; idx++) {
					*(plong++)=longcol;
					*(plong++)=longcol;
					*(plong++)=longcol;
					*(plong++)=longcol;
				}
			}
		}

		if (!bDisableBlank) {
			if (!(VDPREG[1] & 0x40)) {	// Disable display
				return;
			}
		}

		if ((!bDisableBackground) && (gfxline < 192) && (gfxline >= 0)) {
            for (int isLayer2=0; isLayer2<2; ++isLayer2) {
                reg0 = gettables(isLayer2);

                if ((VDPREG[1] & 0x18)==0x18)	// MODE BITS 2 and 1
			    {
				    VDPillegal(gfxline, isLayer2);
			    } else if (VDPREG[1] & 0x10)			// MODE BIT 2
			    {
				    if (reg0 & 0x02) {			// BITMAP MODE BIT
					    VDPtextII(gfxline, isLayer2);	// undocumented bitmap text mode
				    } else if (reg0 & 0x04) {	// MODE BIT 4 (9938)
					    VDPtext80(gfxline, isLayer2);	// 80-column text, similar to 9938/F18A
				    } else {
					    VDPtext(gfxline, isLayer2);		// regular 40-column text
				    }
			    } else if (VDPREG[1] & 0x08)				// MODE BIT 1
			    {
				    if (reg0 & 0x02) {				// BITMAP MODE BIT
					    VDPmulticolorII(gfxline, isLayer2);	// undocumented bitmap multicolor mode
				    } else {
					    VDPmulticolor(gfxline, isLayer2);
				    }
			    } else if (reg0 & 0x02) {					// BITMAP MODE BIT
				    VDPgraphicsII(gfxline, isLayer2);		// documented bitmap graphics mode
			    } else {
				    VDPgraphics(gfxline, isLayer2);
			    }

                // Tile layer 2, if applicable
                // TODO: sprite priority is not taken into account
			    if ((!bF18AActive) || ((VDPREG[49]&0x80)==0)) {
                    break;
                }
            }
		} else {
            // This case is hit if nothing else is being drawn, otherwise the graphics modes call DrawSprites
			// as long as mode bit 2 is not set, sprites are okay
			if ((bF18AActive) || ((VDPREG[1] & 0x10) == 0)) {
				DrawSprites(gfxline);
			}
		}
	} else {
		// we have to redraw the sprites even if the screen didn't change, so that collisions are updated
		// as the CPU may have cleared the collision bit
		// as long as mode bit 2 (text) is not set, and the display is enabled, sprites are okay
		if ((bF18AActive) || ((VDPREG[1] & 0x10) == 0)) {
			if ((bDisableBlank) || (VDPREG[1] & 0x40)) {
				DrawSprites(gfxline);
			}
		}
	}
}

// for the sake of overdrive, force out a single frame
void vdpForceFrame() {
	updateVDP(FULLFRAME);
}

//////////////////////////////////////////////////////
// Draw a debug screen 
//////////////////////////////////////////////////////
void draw_debug()
{
	if (NULL != dbgWnd) {
		SetEvent(hDebugWindowUpdateEvent);
	}
}

/////////////////////////////////////////////////////////
// Draw graphics mode
// Layer 2 for F18A second tile layer!
/////////////////////////////////////////////////////////
void VDPgraphics(int scanline, int isLayer2)
{
	int t,o;				// temp variables
	int i2;					// temp variables
	int p_add;
	int fgc, bgc, c;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

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
				            if (t&80) pixel(i2,i1+i3,fgc);
				            if (t&40) pixel(i2+1,i1+i3,fgc);
				            if (t&20) pixel(i2+2,i1+i3,fgc);
				            if (t&10) pixel(i2+3,i1+i3,fgc);
				            if (t&8) pixel(i2+4,i1+i3,fgc);
				            if (t&4) pixel(i2+5,i1+i3,fgc);
				            if (t&2) pixel(i2+6,i1+i3,fgc);
				            if (t&1) pixel(i2+7,i1+i3,fgc);
                        }
			        }
                } else {
    			    pixel(i2,i1+i3,(t&0x80 ? fgc : bgc ));
				    pixel(i2+1,i1+i3,(t&0x40 ? fgc : bgc ));
				    pixel(i2+2,i1+i3,(t&0x20 ? fgc : bgc ));
				    pixel(i2+3,i1+i3,(t&0x10 ? fgc : bgc ));
				    pixel(i2+4,i1+i3,(t&0x08 ? fgc : bgc ));
				    pixel(i2+5,i1+i3,(t&0x04 ? fgc : bgc ));
				    pixel(i2+6,i1+i3,(t&0x02 ? fgc : bgc ));
				    pixel(i2+7,i1+i3,(t&0x01 ? fgc : bgc ));
                }
			}
		}
	}

	DrawSprites(scanline);

}

/////////////////////////////////////////////////////////
// Draw bitmap graphics mode
/////////////////////////////////////////////////////////
void VDPgraphicsII(int scanline, int isLayer2)
{
	int t,o;				// temp variables
	int i2;					// temp variables
	int p_add, c_add;
	int fgc, bgc;
	int table, Poffset, Coffset;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

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
					pixel(i2,i1+i3,(t&0x80 ? fgc : bgc ));
					pixel(i2+1,i1+i3,(t&0x40 ? fgc : bgc ));
					pixel(i2+2,i1+i3,(t&0x20 ? fgc : bgc ));
					pixel(i2+3,i1+i3,(t&0x10 ? fgc : bgc ));
					pixel(i2+4,i1+i3,(t&0x08 ? fgc : bgc ));
					pixel(i2+5,i1+i3,(t&0x04 ? fgc : bgc ));
					pixel(i2+6,i1+i3,(t&0x02 ? fgc : bgc ));
					pixel(i2+7,i1+i3,(t&0x01 ? fgc : bgc ));
				}
			}
		}
	}

	DrawSprites(scanline);

}

////////////////////////////////////////////////////////////////////////
// Draw text mode 40x24
////////////////////////////////////////////////////////////////////////
void VDPtext(int scanline, int isLayer2)
{ 
	int t,o;
	int i2;
	int fgc, bgc, p_add;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

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

			p_add=PDT+(ch<<3)+i3;
			o++;

            if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                // for layer 2, we have to drop transparent pixels,
                // but I don't want to slow down the normal draw that much
			    //for (i3=0; i3<8; i3++)
			    {	
				    t=VDP[p_add];
                    if (fgc != 0) {     // skip if fgc is also transparent
				        if (t&0x80) pixel(i2,i1+i3,fgc);
				        if (t&0x40) pixel(i2+1,i1+i3,fgc);
				        if (t&0x20) pixel(i2+2,i1+i3,fgc);
				        if (t&0x10) pixel(i2+3,i1+i3,fgc);
				        if (t&0x8) pixel(i2+4,i1+i3,fgc);
				        if (t&0x4) pixel(i2+5,i1+i3,fgc);
                    }
			    }
            } else {
    //			for (i3=0; i3<8; i3++)		// 6 pixels wide
			    {	
				    t=VDP[p_add];
				    pixel(i2,i1+i3,  (t&0x80 ? fgc : bgc ));
				    pixel(i2+1,i1+i3,(t&0x40 ? fgc : bgc ));
				    pixel(i2+2,i1+i3,(t&0x20 ? fgc : bgc ));
				    pixel(i2+3,i1+i3,(t&0x10 ? fgc : bgc ));
				    pixel(i2+4,i1+i3,(t&0x08 ? fgc : bgc ));
				    pixel(i2+5,i1+i3,(t&0x04 ? fgc : bgc ));
			    }
            }
		}
	}

    // no sprites in text mode, unless f18A unlocked
    if ((bF18AActive) && (!isLayer2)) {
        // todo: layer 2 has sprite dependency concerns
	    DrawSprites(scanline);
    }
}

////////////////////////////////////////////////////////////////////////
// Draw bitmap text mode 40x24
////////////////////////////////////////////////////////////////////////
void VDPtextII(int scanline, int isLayer2)
{ 
	int t,o;
	int i2;
	int fgc,bgc, p_add;
	int table, Poffset;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

	o=(scanline/8)*40;							// offset in SIT

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
//			table++;
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
				        if (t&0x80) pixel80(i2,i1+i3,fgc);
				        if (t&0x40) pixel80(i2+1,i1+i3,fgc);
				        if (t&0x20) pixel80(i2+2,i1+i3,fgc);
				        if (t&0x10) pixel80(i2+3,i1+i3,fgc);
				        if (t&0x8) pixel80(i2+4,i1+i3,fgc);
				        if (t&0x4) pixel80(i2+5,i1+i3,fgc);
                    }
			    }
            } else {
    //			for (i3=0; i3<8; i3++)		// 6 pixels wide
			    {	
				    t=VDP[p_add];
				    pixel(i2,i1+i3,(t&0x80 ?   fgc : bgc ));
				    pixel(i2+1,i1+i3,(t&0x40 ? fgc : bgc ));
				    pixel(i2+2,i1+i3,(t&0x20 ? fgc : bgc ));
				    pixel(i2+3,i1+i3,(t&0x10 ? fgc : bgc ));
				    pixel(i2+4,i1+i3,(t&0x08 ? fgc : bgc ));
				    pixel(i2+5,i1+i3,(t&0x04 ? fgc : bgc ));
			    }
            }
		}
	}
	// no sprites in text mode
}

////////////////////////////////////////////////////////////////////////
// Draw text mode 80x24 (note: 80x26.5 mode not supported, blink not supported)
////////////////////////////////////////////////////////////////////////
void VDPtext80(int scanline, int isLayer2)
{ 
	int t,o;
	int i2;
	int fgc,bgc, p_add;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;

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

			p_add=PDT+(ch<<3)+i3;
			o++;

            if ((isLayer2)&&((fgc==0)||(bgc==0))) {
                // for layer 2, we have to drop transparent pixels,
                // but I don't want to slow down the normal draw that much
			    //for (i3=0; i3<8; i3++)
			    {	
				    t=VDP[p_add];
                    if (fgc != 0) {     // skip if fgc is also transparent
				        if (t&0x80) pixel80(i2,i1+i3,fgc);
				        if (t&0x40) pixel80(i2+1,i1+i3,fgc);
				        if (t&0x20) pixel80(i2+2,i1+i3,fgc);
				        if (t&0x10) pixel80(i2+3,i1+i3,fgc);
				        if (t&0x8) pixel80(i2+4,i1+i3,fgc);
				        if (t&0x4) pixel80(i2+5,i1+i3,fgc);
                    }
			    }
            } else {
                //			for (i3=0; i3<8; i3++)		// 6 pixels wide
			    {	
				    t=VDP[p_add];
				    pixel80(i2,i1+i3,(t&0x80   ? fgc : bgc ));
				    pixel80(i2+1,i1+i3,(t&0x40 ? fgc : bgc ));
				    pixel80(i2+2,i1+i3,(t&0x20 ? fgc : bgc ));
				    pixel80(i2+3,i1+i3,(t&0x10 ? fgc : bgc ));
				    pixel80(i2+4,i1+i3,(t&0x08 ? fgc : bgc ));
				    pixel80(i2+5,i1+i3,(t&0x04 ? fgc : bgc ));
			    }
            }
		}
	}
    // no sprites in text mode, unless f18A unlocked
    // TODO: sprites don't render correctly in the wider 80 column mode...
    if ((bF18AActive) && (!isLayer2)) {
        // todo: layer 2 has sprite dependency concerns
	    DrawSprites(scanline);
    }
}

////////////////////////////////////////////////////////////////////////
// Draw Illegal mode (similar to text mode)
////////////////////////////////////////////////////////////////////////
void VDPillegal(int scanline, int isLayer2)
{ 
	int t;
	int i2;
	int fgc,bgc;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x07;
	(void)scanline;		// scanline is irrelevant

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
			    //for (i3=0; i3<8; i3++)
			    {	
                    // fix it so that bgc is transparent, then we only draw on that test
                    if (fgc!=0) {
				        pixel(i2,i1+i3,fgc);
				        pixel(i2+1,i1+i3,fgc);
				        pixel(i2+2,i1+i3,fgc);
				        pixel(i2+3,i1+i3,fgc);
                    }
                    if (bgc!=0) {
				        pixel(i2+4,i1+i3,bgc);
				        pixel(i2+5,i1+i3,bgc);
                    }
			    }
            } else {
    //			for (i3=0; i3<8; i3++)				// 6 pixels wide
			    {	
				    pixel(i2,i1+i3  ,fgc);
				    pixel(i2+1,i1+i3,fgc);
				    pixel(i2+2,i1+i3,fgc);
				    pixel(i2+3,i1+i3,fgc);
				    pixel(i2+4,i1+i3,bgc);
				    pixel(i2+5,i1+i3,bgc);
			    }
            }
		}
	}
	// no sprites in this mode
}

/////////////////////////////////////////////////////
// Draw Multicolor Mode
/////////////////////////////////////////////////////
void VDPmulticolor(int scanline, int isLayer2) 
{
	int o;				// temp variables
	int i2;				// temp variables
	int p_add;
	int fgc, bgc;
	int off;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x04;
	const int i4 = scanline&0x03;

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
					        pixel(i2,i1+i3+i4,fgc);
					        pixel(i2+1,i1+i3+i4,fgc);
					        pixel(i2+2,i1+i3+i4,fgc);
					        pixel(i2+3,i1+i3+i4,fgc);
                        }
                        if (bgc != 0) {
					        pixel(i2+4,i1+i3+i4,bgc);
					        pixel(i2+5,i1+i3+i4,bgc);
					        pixel(i2+6,i1+i3+i4,bgc);
					        pixel(i2+7,i1+i3+i4,bgc);
                        }
			        }
                } else {
    //				for (i4=0; i4<4; i4++) 
				    {
					    pixel(i2,i1+i3+i4,fgc);
					    pixel(i2+1,i1+i3+i4,fgc);
					    pixel(i2+2,i1+i3+i4,fgc);
					    pixel(i2+3,i1+i3+i4,fgc);
					    pixel(i2+4,i1+i3+i4,bgc);
					    pixel(i2+5,i1+i3+i4,bgc);
					    pixel(i2+6,i1+i3+i4,bgc);
					    pixel(i2+7,i1+i3+i4,bgc);
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

/////////////////////////////////////////////////////
// Draw Bitmap Multicolor Mode
// TODO not proven to be correct anymore
/////////////////////////////////////////////////////
void VDPmulticolorII(int scanline, int isLayer2) 
{
	int o;						// temp variables
	int i2;						// temp variables
	int p_add;
	int fgc, bgc;
	int off;
	int table, Poffset;
	unsigned char ch=0xff;
	const int i1 = scanline&0xf8;
	const int i3 = scanline&0x04;
	const int i4 = scanline&0x03;

	o=(scanline/8)*32;			// offset in SIT
	off=(scanline>>2)&0x06;		// offset in pattern

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
					        pixel(i2,i1+i3+i4,fgc);
					        pixel(i2+1,i1+i3+i4,fgc);
					        pixel(i2+2,i1+i3+i4,fgc);
					        pixel(i2+3,i1+i3+i4,fgc);
                        }
                        if (bgc != 0) {
					        pixel(i2+4,i1+i3+i4,bgc);
					        pixel(i2+5,i1+i3+i4,bgc);
					        pixel(i2+6,i1+i3+i4,bgc);
					        pixel(i2+7,i1+i3+i4,bgc);
                        }
			        }
                } else {
    //				for (i4=0; i4<4; i4++) 
				    {
					    pixel(i2,i1+i3+i4,fgc);
					    pixel(i2+1,i1+i3+i4,fgc);
					    pixel(i2+2,i1+i3+i4,fgc);
					    pixel(i2+3,i1+i3+i4,fgc);
					    pixel(i2+4,i1+i3+i4,bgc);
					    pixel(i2+5,i1+i3+i4,bgc);
					    pixel(i2+6,i1+i3+i4,bgc);
					    pixel(i2+7,i1+i3+i4,bgc);
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
unsigned int* drawTextLine(unsigned int *pDat, const char *buf) {
	for (int idx = 0; idx<(signed)strlen(buf); idx++) {
        if (buf[idx] == '1') {
    		*(pDat++)=0xffffff;
		} else {
			*(pDat++)=0;
		}
	}
    return pDat;
}

////////////////////////////////////////////////////////////////
// Stretch-blit the buffer into the active window
//
// NOTES: Graphics modes we have (and some we need)
// 272x208 -- the standard default pixel mode of the 9918A plus a fixed (incorrect) border
// 
//
// NOTES: Graphics modes we have (and some we need)
// 272x208 -- the standard default pixel mode of the 9918A plus a fixed (incorrect) border
// 544x413 -- the double-sized filters (minus 1 scanline due to corruption)
// 1088x832 - HQ4x buffer
// 634x413 -- TV mode
// 528x208 -- 80-column mode
// These are all with 24 rows -- the F18A adds a 26.5 row mode (212 pixels) (todo: or was it 240?)
// So this adds another 20 (or 48) pixels to each mode
// One solution might be to simply render a fixed TV display and scale to fit...
// The only place it really matters if resolution changes is video recording.
// In that case, the vertical can always be the same - the extra rows just cut into overscan.
// Horizontal, unscaled, is either 272, 528 or 634. We could adapt a buffer size that
// fits all, maybe, and just adjust the amount of overscan area...?
// Alternately... maybe we just blit whatever into a fixed size video buffer (say, 2x) and be done?
////////////////////////////////////////////////////////////////
void doBlit()
{
	RECT rect1, rect2;
	int x,y;
	HRESULT ret;

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
		for (int i2=0; i2<5; i2++) {
			unsigned int *pDat = framedata + (256+16)*(6-i2);
			for (int idx = 0; idx<(signed)strlen(buf); idx++) {
                int digit = buf[idx] - '0';
                pDat = drawTextLine(pDat, digpat[digit][i2]);
				*(pDat++)=0;
			}
		}
	}
    if (bShowKeyboard) {
		// draw digits
        const char *caps[5] = {
            "111  1  111",
            "1   1 1 1 1",
            "1   111 111",
            "1   1 1 1  ",
            "111 1 1 1  "   };
        const char *lock[5] = {
            "1   111 111 1 1",
            "1   1 1 1   11 ",
            "1   1 1 1   11 ",
            "1   1 1 1   1 1",
            "111 111 111 1 1"   };
        const char *scrl[5] = {
            "111 111 111 1  ",
            "1   1   1 1 1  ",
            "111 1   11  1  ",
            "  1 1   1 1 1  ",
            "111 111 1 1 111"   };
        const char *num[5] = {
            "1 1 1 1 1 1",
            "111 1 1 111",
            "111 1 1 111",
            "111 1 1 1 1",
            "1 1 111 1 1"   };
        const char *joy[5] = {
            "  1 111 1 1",
            "  1 1 1 1 1",
            "  1 1 1 111",
            "1 1 1 1  1 ",
            "111 111  1 "   };
        const char *ign[5] = {
            "111 111 1 1",
            " 1  1   111",
            " 1  1 1 111",
            " 1  1 1 111",
            "111 111 1 1"   };
        const char *fctn[5] = {
            "111 111 111 1 1",
            "1   1    1  111",
            "11  1    1  111",
            "1   1    1  111",
            "1   111  1  1 1"   };
        const char *shift[5] = {
            "111 1 1 111 111",
            "1   1 1 1    1 ",
            "111 111 11   1 ",
            "  1 1 1 1    1 ",
            "111 1 1 1    1 "   };
        const char *ctrl[5] = {
            "111 111 111 1  ",
            "1    1  1 1 1  ",
            "1    1  11  1  ",
            "1    1  1 1 1  ",
            "111  1  1 1 111"   };
		char buf[32];

		for (int i2=0; i2<5; i2++) {
			unsigned int *pDat = framedata + (256+16)*(6-i2)+20;

            if (capslock) {
                drawTextLine(pDat, caps[i2]);
			}
            pDat += 20;

            if (lockedshiftstate) {
                drawTextLine(pDat, lock[i2]);
			}
            pDat += 20;

            if (scrolllock) {
                drawTextLine(pDat, scrl[i2]);
			}
            pDat += 20;

            if (numlock) {
                drawTextLine(pDat, num[i2]);
			}
            pDat += 20;
        
            if (fJoystickActiveOnKeys) {
                drawTextLine(pDat, joy[i2]);
			}
            pDat += 20;

            pDat = drawTextLine(pDat, ign[i2]);
            *(pDat++) = 0;
            sprintf(buf, "%d", ignorecount);
			for (int idx = 0; idx<(signed)strlen(buf); idx++) {
                int digit = buf[idx] - '0';
                pDat = drawTextLine(pDat, digpat[digit][i2]);
				*(pDat++)=0;
			}
            pDat+=4;

            pDat = drawTextLine(pDat, fctn[i2]);
            *(pDat++) = 0;
            sprintf(buf, "%d", fctnrefcount);
			for (int idx = 0; idx<(signed)strlen(buf); idx++) {
                int digit = buf[idx] - '0';
                pDat = drawTextLine(pDat, digpat[digit][i2]);
				*(pDat++)=0;
			}
            pDat+=4;

            pDat = drawTextLine(pDat, shift[i2]);
            *(pDat++) = 0;
            sprintf(buf, "%d", shiftrefcount);
			for (int idx = 0; idx<(signed)strlen(buf); idx++) {
                int digit = buf[idx] - '0';
                pDat = drawTextLine(pDat, digpat[digit][i2]);
				*(pDat++)=0;
			}
            pDat+=4;
            
            pDat = drawTextLine(pDat, ctrl[i2]);
            *(pDat++) = 0;
            sprintf(buf, "%d", ctrlrefcount);
			for (int idx = 0; idx<(signed)strlen(buf); idx++) {
                int digit = buf[idx] - '0';
                pDat = drawTextLine(pDat, digpat[digit][i2]);
				*(pDat++)=0;
			}
		}

        for (int i2=0; i2<8; i2++) {
			unsigned int *pDat = framedata + (256+16)*(9-i2)+220;
            for (int mask=1; mask<0x100; mask<<=1) {
                *(pDat++) = (ticols[i2]&mask) ? 0xffffff : 0;
                *(pDat++) = 0xffffff;
            }
        }

	}


	if (!TryEnterCriticalSection(&VideoCS)) {
		return;		// do it later
	}

	GetClientRect(myWnd, &rect1);
	myDC=GetDC(myWnd);
	SetStretchBltMode(myDC, COLORONCOLOR);

	// TODO: hacky city - 80-column mode doesn't filter or anything, cause we'd have to change ALL the stuff below.
	if ((bEnable80Columns)&&(VDPREG[0]&0x04)&&(VDPREG[1]&0x10)) {
		// render 80 columns to the screen using DIB blit
		StretchDIBits(myDC, 0, 0, rect1.right-rect1.left, rect1.bottom-rect1.top, 0, 0, 512+16, 192+16, framedata, &myInfo80Col, 0, SRCCOPY);
		ReleaseDC(myWnd, myDC);
		LeaveCriticalSection(&VideoCS);
		return;
	}

	// make sure filters work before calling them
	if (FilterMode == 4) {
		if ((!TVFiltersAvailable) || (NULL == sms_ntsc_blit)) {
			MessageBox(myWnd, "Filter DLL not available - reverting to no filter.", "Classic99 Error", MB_OK);
			PostMessage(myWnd, WM_COMMAND, ID_VIDEO_FILTERMODE_NONE, 0);
			ReleaseDC(myWnd, myDC);
			LeaveCriticalSection(&VideoCS);
			return;
		}
	}
	if (FilterMode == 5) {
		if ((NULL == hHQ4DLL) || (NULL == hq4x_init)) {
			MessageBox(myWnd, "HQ4 DLL not available - reverting to no filter.", "Classic99 Error", MB_OK);
			PostMessage(myWnd, WM_COMMAND, ID_VIDEO_FILTERMODE_NONE, 0);
			ReleaseDC(myWnd, myDC);
			LeaveCriticalSection(&VideoCS);
			return;
		}
	}

	// Do the filtering - we throw away the top and bottom 3 scanlines due to some garbage there - it's border anyway
	switch (FilterMode) {
	case 1: // 2xSaI
		_2xSaI((uint8*) framedata+((256+16)*4), ((256+16)*4), NULL, (uint8*)framedata2, (512+32)*4, 256+16, 191+16);
		break;
	case 2: // Super2xSaI
		Super2xSaI((uint8*) framedata+((256+16)*4), ((256+16)*4), NULL, (uint8*)framedata2, (512+32)*4, 256+16, 191+16);
		break;
	case 3: // SuperEagle
		SuperEagle((uint8*) framedata+((256+16)*4), ((256+16)*4), NULL, (uint8*)framedata2, (512+32)*4, 256+16, 191+16);
		break;
	case 4:	// TV filter
		// This filter outputs 602 pixels for 256 in. What we should do is resize the window
		// we eventually produce a TV_WIDTH x 384+29 image (leaving vertical the same)
		sms_ntsc_blit(&tvFilter, framedata, 256+16, 256+16, 192+16, framedata2, (TV_WIDTH)*2*4);
		if (TVScanLines) {
			sms_ntsc_scanlines(framedata2, TV_WIDTH, (TV_WIDTH)*4, 384+29);
		} else {
			// Duplicate every line instead
			for (int y=1; y<384+29; y+=2) {
				memcpy(&framedata2[y*TV_WIDTH], &framedata2[(y-1)*TV_WIDTH], sizeof(framedata2[0])*TV_WIDTH);
			}
		}
		break;
	case 5:	// HQ4x filter - super hi-def!
		{
			if (NULL != hq4x_process) {
				hq4x_process((unsigned char*)framedata, (unsigned char*)framedata2);
			}
		}
	}

	switch (StretchMode) {
	case 1:	// DIB
		switch (FilterMode) {
		case 0:		// none
			StretchDIBits(myDC, 0, 0, rect1.right-rect1.left, rect1.bottom-rect1.top, 0, 0, 256+16, 192+16, framedata, &myInfo, 0, SRCCOPY);
			break;

		case 4:		// TV
			StretchDIBits(myDC, 0, 0, rect1.right-rect1.left, rect1.bottom-rect1.top, 0, 0, TV_WIDTH, 384+29, framedata2, &myInfoTV, 0, SRCCOPY);
			break;

		case 5:		// hq4x
			StretchDIBits(myDC, 0, 0, rect1.right-rect1.left, rect1.bottom-rect1.top, 0, 0, (256+16)*4, (192+16)*4, framedata2, &myInfo32, 0, SRCCOPY);
			break;

		default:	// all the SAI ones
			StretchDIBits(myDC, 0, 0, rect1.right-rect1.left, rect1.bottom-rect1.top, 0, 0, 512+32, 384+29, framedata2, &myInfo2, 0, SRCCOPY);
		}
		break;

	case 2: // DX
		if (NULL == lpdd) {
			SetupDirectDraw(0);
			if (NULL == lpdd) {
				StretchMode=0;
				break;
			}
		}

		if (DD_OK != lpdd->TestCooperativeLevel()) {
			break;
		}

		if (NULL == ddsBack) {
			StretchMode=0;
			break;
		}

		if (DD_OK == ddsBack->GetDC(&tmpDC)) {	// color depth translation
			switch (FilterMode) {
				case 0:
					// original buffer
					SetDIBitsToDevice(tmpDC, 0, 0, 256+16, 192+16, 0, 0, 0, 192+16, framedata, &myInfo, DIB_RGB_COLORS);
					break;

				case 4:
					// TV buffer
					SetDIBitsToDevice(tmpDC, 0, 0, TV_WIDTH, 384+29, 0, 0, 0, 384+29, framedata2, &myInfoTV, DIB_RGB_COLORS);
					break;
				
				case 5:
					// 4x buffer
					SetDIBitsToDevice(tmpDC, 0, 0, (256+16)*4, (192+16)*4, 0, 0, 0, (192+16)*4, framedata2, &myInfo32, DIB_RGB_COLORS);
					break;

				default:
					// 2x buffer
					SetDIBitsToDevice(tmpDC, 0, 0, 512+32, 384+29, 0, 0, 0, 384+29, framedata2, &myInfo2, DIB_RGB_COLORS);
					break;
			}
		}
		ddsBack->ReleaseDC(tmpDC);
		GetWindowRect(myWnd, &rect2);
		// rect1 contains client coordinates (with the correct size!)
		// rect2 contains window coordinates

		POINT pt;
		pt.x = 0;
		pt.y = 0;
		ClientToScreen(myWnd, &pt);
		rect1.top = pt.y;
		rect1.bottom += pt.y;
		rect1.left = pt.x;
		rect1.right+= pt.x;

		// The DirectDraw blit will draw using screen coordinates but into the client area thanks to the clipper
		if (DDERR_SURFACELOST == lpdds->Blt(&rect1, ddsBack, NULL, DDBLT_DONOTWAIT, NULL)) {	// Just go as quick as we can, don't bother waiting
			lpdd->RestoreAllSurfaces();
		}
		break;

	case 3: // DX Full
		if (NULL == lpdd) {
			SetupDirectDraw(FullScreenMode);
			if (NULL == lpdd) {
				StretchMode=0;
				break;
			}
		}

		if (DD_OK != lpdd->TestCooperativeLevel()) {
			break;
		}
		
		if (NULL == ddsBack) {
			StretchMode=0;
			break;
		}
		if (DD_OK == ddsBack->GetDC(&tmpDC)) {	// color depth translation
			switch (FilterMode) {
				case 0:		// none
					SetDIBitsToDevice(tmpDC, 0, 0, 256+16, 192+16, 0, 0, 0, 192+16, framedata, &myInfo, DIB_RGB_COLORS);
					break;

				case 4:		// tv
					SetDIBitsToDevice(tmpDC, 0, 0, TV_WIDTH, 384+29, 0, 0, 0, 384+29, framedata2, &myInfoTV, DIB_RGB_COLORS);
					break;

				case 5:		// hq4x
					SetDIBitsToDevice(tmpDC, 0, 0, (256+16)*4, (192+16)*4, 0, 0, 0, (192+16)*4, framedata2, &myInfo32, DIB_RGB_COLORS);
					break;

				default:	// 2x
					SetDIBitsToDevice(tmpDC, 0, 0, 512+32, 384+29, 0, 0, 0, 384+29, framedata2, &myInfo2, DIB_RGB_COLORS);
					break;
			}
		}
		ddsBack->ReleaseDC(tmpDC);
		if (DD_OK != (ret=lpdds->Blt(NULL, ddsBack, NULL, DDBLT_DONOTWAIT, NULL))) {
			if (DDERR_SURFACELOST == ret) {
				lpdd->RestoreAllSurfaces();
			}
		}
		break;

	default:// None
		// Center it in the window, whatever size
		switch (FilterMode) {
		case 0:		// none
			x=(rect1.right-rect1.left-(256+16))/2;
			y=(rect1.bottom-rect1.top-(192+16))/2;
			x=SetDIBitsToDevice(myDC, x, y, 256+16, 192+16, 0, 0, 0, 192+16, framedata, &myInfo, DIB_RGB_COLORS);
			y=GetLastError();
			break;
		
		case 4:		// TV
			x=(rect1.right-rect1.left-(TV_WIDTH))/2;
			y=(rect1.bottom-rect1.top-(384+29))/2;
			SetDIBitsToDevice(myDC, x, y, TV_WIDTH, 384+29, 0, 0, 0, 384+29, framedata2, &myInfoTV, DIB_RGB_COLORS);
			break;

		case 5:		// hq4x
			x=(rect1.right-rect1.left-(256+16)*4)/2;
			y=(rect1.bottom-rect1.top-(192+16)*4)/2;
			x=SetDIBitsToDevice(myDC, x, y, (256+16)*4, (192+16)*4, 0, 0, 0, (192+16)*4, framedata2, &myInfo32, DIB_RGB_COLORS);
			y=GetLastError();
			break;

		default:	// 2x
			x=(rect1.right-rect1.left-(512+32))/2;
			y=(rect1.bottom-rect1.top-(384+29))/2;
			SetDIBitsToDevice(myDC, x, y, 512+32, 384+29, 0, 0, 0, 384+29, framedata2, &myInfo2, DIB_RGB_COLORS);
			break;
		}
		break;
	}

	ReleaseDC(myWnd, myDC);

	LeaveCriticalSection(&VideoCS);
}

//////////////////////////////////////////////////////////
// Draw Sprites into the backbuffer
//////////////////////////////////////////////////////////
void DrawSprites(int scanline)
{
	int i1, i2, i3, xx, yy, pat, col, p_add, t, sc;
	int highest;
	int curSAL;

	// a hacky, but effective 4-sprite-per-line limitation emulation
	// We can do this right when we have scanline based VDP (TODO: or even later)
	char nLines[192];
	char bSkipScanLine[32][32];		// 32 sprites, 32 lines max
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
	// TODO: and do all the above properly, now that we're scanline based
	int b5OnLine=-1;

	if (bDisableSprite) {
		return;
	}

	// check if b5OnLine is already latched, and set it if so.
	if (VDPS & VDPS_5SPR) {
		b5OnLine = VDPS & 0x1f;
	}

	memset(nLines, 0, sizeof(nLines));
	memset(bSkipScanLine, 0, sizeof(bSkipScanLine));

	// set up the draw
	memset(SprColBuf, 0, 256*192);
	SprColFlag=0;
	
	highest=31;

	// find the highest active sprite
	for (i1=0; i1<32; i1++)			// 32 sprites 
	{
		yy=VDP[SAL+(i1<<2)];
		if (yy==0xd0)
		{
			highest=i1-1;
			break;
		}
	}
	
	if (bUse5SpriteLimit) {
		// go through the sprite table and check if any scanlines are obliterated by 4-per-line
		i3=8;							// number of sprite scanlines
		if (VDPREG[1] & 0x2) {			 // TODO: Handle F18A ECM where sprites are doubled individually
			// double-sized
			i3*=2;
		}
		if (VDPREG[1]&0x01)	{
			// magnified sprites
			i3*=2;
		}
        int max = 5;                    // 9918A - fifth sprite is lost
        if (bF18AActive) {
            max = VDPREG[0x33];         // F18A - configurable value
            if (max == 0) max = 5;      // assume jumper set to 9918A mode
        }
		for (i1=0; i1<=highest; i1++) {
			curSAL=SAL+(i1<<2);
			yy=VDP[curSAL]+1;				// sprite Y, it's stupid, cause 255 is line 0 
			if (yy>225) yy-=256;			// fade in from top
			t=yy;
			for (i2=0; i2<i3; i2++,t++) {
				if ((t>=0) && (t<=191)) {
					nLines[t]++;
					if (nLines[t]>=max) {
						if (t == scanline) {
							if (b5OnLine == -1) b5OnLine=i1;
						}
						bSkipScanLine[i1][i2]=1;
					}
				}
			}
		}
	}

	// now draw
	for (i1=highest; i1>=0; i1--)	
	{	
		curSAL=SAL+(i1<<2);
		yy=VDP[curSAL++]+1;				// sprite Y, it's stupid, cause 255 is line 0 
		if (yy>225) yy-=256;			// fade in from top: TODO: is this right??
		xx=VDP[curSAL++];				// sprite X 
		pat=VDP[curSAL++];				// sprite pattern
		int dblSize = F18AECModeSprite ? VDP[curSAL] & 0x10 : VDPREG[1] & 0x2;
		if (dblSize) {
			pat=pat&0xfc;				// if double-sized, it must be a multiple of 4
		}
		col=VDP[curSAL]&0xf;			// sprite color 
	
		if (VDP[curSAL++]&0x80)	{		// early clock
			xx-=32;
		}

		// Even transparent sprites get drawn into the collision buffer
		p_add=SDT+(pat<<3);
		sc=0;						// current scanline
		
		// Added by Rasmus M
		// TODO: For ECM 1 we need one more bit from R24 (Mike: is that ECM? I think it's always!)
		int paletteBase = F18AECModeSprite ? (col >> (F18AECModeSprite - 2)) * F18ASpritePaletteSize : 0;
		int F18ASpriteColorLine[8]; // Colors indices for each of the 8 pixels in a sprite scan line

		if (VDPREG[1]&0x01)	{		// magnified sprites
			for (i3=0; i3<16; i3++)
			{	
				t = pixelMask(p_add, F18ASpriteColorLine);	// Modified by RasmusM. Sets up the F18ASpriteColorLine[] array.

				if ((!bSkipScanLine[i1][sc]) && (yy+i3 == scanline)) {
					if (t&0x80) 
						bigpixel(xx, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
					if (t&0x40)
						bigpixel(xx+2, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
					if (t&0x20)
						bigpixel(xx+4, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
					if (t&0x10)
						bigpixel(xx+6, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
					if (t&0x08)
						bigpixel(xx+8, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
					if (t&0x04)
						bigpixel(xx+10, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
					if (t&0x02)
						bigpixel(xx+12, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
					if (t&0x01)
						bigpixel(xx+14, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
				}

				if (dblSize)		// double-size sprites, need to draw 3 more chars 
				{	
					t = pixelMask(p_add + 8, F18ASpriteColorLine);	// Modified by RasmusM
	
					if ((!bSkipScanLine[i1][sc+16]) && (yy+i3+16 == scanline)) {
						if (t&0x80)
							bigpixel(xx, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							bigpixel(xx+2, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							bigpixel(xx+4, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							bigpixel(xx+6, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							bigpixel(xx+8, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							bigpixel(xx+10, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							bigpixel(xx+12, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							bigpixel(xx+14, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);

						t = pixelMask(p_add + 24, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							bigpixel(xx+16, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							bigpixel(xx+18, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							bigpixel(xx+20, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							bigpixel(xx+22, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							bigpixel(xx+24, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							bigpixel(xx+26, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							bigpixel(xx+28, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							bigpixel(xx+30, yy+i3+16, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}

					if ((!bSkipScanLine[i1][sc]) && (yy+i3 == scanline)) {
						t = pixelMask(p_add + 16, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							bigpixel(xx+16, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							bigpixel(xx+18, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							bigpixel(xx+20, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)	
							bigpixel(xx+22, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							bigpixel(xx+24, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							bigpixel(xx+26, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							bigpixel(xx+28, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							bigpixel(xx+30, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}
				}
				sc++;
				p_add += i3&0x01;
			}
		} else {
			for (i3=0; i3<8; i3++)
			{	
				t = pixelMask(p_add++, F18ASpriteColorLine);	// Modified by RasmusM

				if ((!bSkipScanLine[i1][sc]) && (yy+i3 == scanline)) {
					if (t&0x80)
						spritepixel(xx, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
					if (t&0x40)
						spritepixel(xx+1, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
					if (t&0x20)
						spritepixel(xx+2, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
					if (t&0x10)
						spritepixel(xx+3, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
					if (t&0x08)
						spritepixel(xx+4, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
					if (t&0x04)
						spritepixel(xx+5, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
					if (t&0x02)
						spritepixel(xx+6, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
					if (t&0x01)
						spritepixel(xx+7, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
				}

				if (dblSize)		// double-size sprites, need to draw 3 more chars 
				{	
					t = pixelMask(p_add + 7, F18ASpriteColorLine);	// Modified by RasmusM

					if ((!bSkipScanLine[i1][sc+8]) && (yy+i3+8 == scanline)) {
						if (t&0x80)
							spritepixel(xx, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							spritepixel(xx+1, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							spritepixel(xx+2, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							spritepixel(xx+3, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							spritepixel(xx+4, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							spritepixel(xx+5, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							spritepixel(xx+6, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							spritepixel(xx+7, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);

						t = pixelMask(p_add + 23, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							spritepixel(xx+8, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							spritepixel(xx+9, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							spritepixel(xx+10, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)
							spritepixel(xx+11, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							spritepixel(xx+12, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							spritepixel(xx+13, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							spritepixel(xx+14, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							spritepixel(xx+15, yy+i3+8, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}

					if ((!bSkipScanLine[i1][sc]) && (yy+i3 == scanline)) {
						t = pixelMask(p_add + 15, F18ASpriteColorLine);	// Modified by RasmusM
						if (t&0x80)
							spritepixel(xx+8, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[0] : col);
						if (t&0x40)
							spritepixel(xx+9, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[1] : col);
						if (t&0x20)
							spritepixel(xx+10, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[2] : col);
						if (t&0x10)	
							spritepixel(xx+11, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[3] : col);
						if (t&0x08)
							spritepixel(xx+12, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[4] : col);
						if (t&0x04)
							spritepixel(xx+13, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[5] : col);
						if (t&0x02)
							spritepixel(xx+14, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[6] : col);
						if (t&0x01)
							spritepixel(xx+15, yy+i3, F18AECModeSprite ? paletteBase + F18ASpriteColorLine[7] : col);
					}
				}
				sc++;
			}
		}
	}
	// Set the VDP collision bit
	if (SprColFlag) {
		VDPS|=VDPS_SCOL;
	}
	if (b5OnLine != -1) {
		VDPS|=VDPS_5SPR;
		VDPS&=(VDPS_INT|VDPS_5SPR|VDPS_SCOL);
		VDPS|=b5OnLine&(~(VDPS_INT|VDPS_5SPR|VDPS_SCOL));
	} else {
		VDPS&=(VDPS_INT|VDPS_5SPR|VDPS_SCOL);
		// TODO: so if all 32 sprites are in used, the 5-on-line value for not
		// having 5 on a line anywhere will be 0? (0x1f+1)=0x20 -> 0x20&0x1f = 0!
		// The correct behaviour is probably to count in realtime - that goes in with
		// the scanline code.
		VDPS|=(highest+1)&(~(VDPS_INT|VDPS_5SPR|VDPS_SCOL));
	}
}

////////////////////////////////////////////////////////////
// Draw a pixel onto the backbuffer surface
////////////////////////////////////////////////////////////
void pixel(int x, int y, int c)
{
	if ((x > 255)||(y>192)) {
		debug_write("here");
	}

	framedata[((199-y)<<8)+((199-y)<<4)+x+8]=GETPALETTEVALUE(c);
}

////////////////////////////////////////////////////////////
// Draw a pixel onto the backbuffer surface in 80 column mode
////////////////////////////////////////////////////////////
void pixel80(int x, int y, int c)
{
	framedata[((199-y)<<9)+((199-y)<<4)+x+8]=GETPALETTEVALUE(c);
}

////////////////////////////////////////////////////////////
// Draw a range-checked pixel onto the backbuffer surface
////////////////////////////////////////////////////////////
void spritepixel(int x, int y, int c)
{
	if ((y>191)||(y<0)) return;
    if ((VDPREG[1] & 0x10) == 0) {
        // normal modes
        if ((x>255)||(x<0)) return;
    } else {
        // text mode - we only get here in the F18A case, and it truncates on the text coordinates
        if ((x>=248)||(x<8)) return;
    }
	
	if (SprColBuf[x][y]) {
		SprColFlag=1;
	} else {
		SprColBuf[x][y]=1;
	}

	if (!(F18AECModeSprite ? c % F18ASpritePaletteSize : c)) return;		// don't DRAW transparent, Modified by RasmusM
	// TODO: this is probably okay but needs to be cleaned up with removal of TIPALETTE - note we do NOT use GETPALETTEVALUE
	// here because the palette index was calculated for full ECM sprites
	framedata[((199-y)<<8)+((199-y)<<4)+x+8] = F18APalette[c];	// Modified by RasmusM
	return;
}

////////////////////////////////////////////////////////////
// Draw a magnified pixel onto the backbuffer surface
////////////////////////////////////////////////////////////
void bigpixel(int x, int y, int c)
{
	spritepixel(x,y,c);
	spritepixel(x+1,y,c);
//	spritepixel(x,y+1,c);
//	spritepixel(x+1,y+1,c);
}

////////////////////////////////////////////////////////////
// Pixel mask
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

////////////////////////////////////////////////////////////
// DirectX full screen enumeration callback
////////////////////////////////////////////////////////////
HRESULT WINAPI myCallBack(LPDDSURFACEDESC2 ddSurface, LPVOID pData) {
	int *c;

	c=(int*)pData;

	if (ddSurface->ddpfPixelFormat.dwRGBBitCount == (DWORD)*c) {
		*c=(*c)|0x80;
		return DDENUMRET_CANCEL;
	}
	return DDENUMRET_OK;
}

////////////////////////////////////////////////////////////
// Setup DirectDraw, with the requested fullscreen mode
// In order for Fullscreen to work, only the main thread
// may call this function!
////////////////////////////////////////////////////////////
void SetupDirectDraw(int fullscreen) {
	int x,y,c;
	RECT myRect;

	EnterCriticalSection(&VideoCS);

	// directdraw is deprecated -- for now we can still do this, but
	// we need to replace this API with Direct3D (TODO)
    HINSTANCE hInstDDraw;
    LPDIRECTDRAWCREATEEX pDDCreate = NULL;

    hInstDDraw = LoadLibrary( "ddraw.dll" );
    if( hInstDDraw == NULL ) {
		MessageBox(myWnd, "Can't load DLL for DirectDraw 7\nClassic99 Requires DirectX 7 for DX and Full screen modes", "Classic99 Error", MB_OK);
		lpdd=NULL;
		StretchMode=0;
		goto optout;
	}

    pDDCreate = ( LPDIRECTDRAWCREATEEX )GetProcAddress( hInstDDraw, "DirectDrawCreateEx" );

	if (pDDCreate(NULL, (void**)&lpdd, IID_IDirectDraw7, NULL)!=DD_OK) {
		MessageBox(myWnd, "Unable to initialize DirectDraw 7\nClassic99 Requires DirectX 7 for DX and Full screen modes", "Classic99 Error", MB_OK);
		lpdd=NULL;
		StretchMode=0;
	} else {
		if (fullscreen) {
			DDSURFACEDESC2 myDesc;

			GetWindowRect(myWnd, &myRect);

			switch (fullscreen) {
				case 1: x=320; y=240; c=8; break;
				case 2: x=640; y=480; c=8; break;
				case 3: x=640; y=480; c=16; break;
				case 4: x=640; y=480; c=32; break;
				case 5: x=800; y=600; c=16; break;
				case 6: x=800; y=600; c=32; break;
				case 7: x=1024; y=768; c=16; break;
				case 8: x=1024; y=768; c=32; break;
				default:x=640; y=480; c=16; break;
			}

			// Check if mode is legal
			ZeroMemory(&myDesc, sizeof(myDesc));
			myDesc.dwSize=sizeof(myDesc);
			myDesc.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
			myDesc.dwWidth=x;
			myDesc.dwHeight=y;
			lpdd->EnumDisplayModes(0, &myDesc, (void*)&c, myCallBack);
			// If a valid mode was found, 'c' has 0x80 ORd with it
			if (0 == (c&0x80)) {
				MessageBox(myWnd, "Requested graphics mode is not supported on the primary display.", "Classic99 Error", MB_OK);
				if (lpdd) lpdd->Release();
				lpdd=NULL;
				StretchMode=0;
				MoveWindow(myWnd, myRect.left, myRect.top, myRect.right-myRect.left, myRect.bottom-myRect.top, true);
				goto optout;
			}

			c&=0x7f;	// Remove the flag bit

			if (lpdd->SetCooperativeLevel(myWnd, DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX)!=DD_OK) {
				MessageBox(myWnd, "Unable to set cooperative level\nFullscreen DX is not available", "Classic99 Error", MB_OK);
				if (lpdd) lpdd->Release();
				lpdd=NULL;
				StretchMode=0;
				MoveWindow(myWnd, myRect.left, myRect.top, myRect.right-myRect.left, myRect.bottom-myRect.top, true);
				goto optout;
			}

			if (lpdd->SetDisplayMode(x,y,c,0,0) != DD_OK) {
				MessageBox(myWnd, "Unable to set display mode.\nRequested DX mode is not available", "Classic99 Error", MB_OK);
				MoveWindow(myWnd, myRect.left, myRect.top, myRect.right-myRect.left, myRect.bottom-myRect.top, true);
				StretchMode=0;
				goto optout;
			}

            // disable the menu
            SetMenuMode(false, false);
		} else {
			if (lpdd->SetCooperativeLevel(myWnd, DDSCL_NORMAL)!=DD_OK) {
				MessageBox(myWnd, "Unable to set cooperative level\nDX mode is not available", "Classic99 Error", MB_OK);
				if (lpdd) lpdd->Release();
				lpdd=NULL;
				StretchMode=0;
				goto optout;
			}

            // enable the menu
            SetMenuMode(true, !bEnableAppMode);
		}

		ZeroMemory(&CurrentDDSD, sizeof(CurrentDDSD));
		CurrentDDSD.dwSize=sizeof(CurrentDDSD);
		CurrentDDSD.dwFlags=DDSD_CAPS;
		CurrentDDSD.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;

		if (lpdd->CreateSurface(&CurrentDDSD, &lpdds, NULL) !=DD_OK) {
			MessageBox(myWnd, "Unable to create primary surface\nDX mode is not available", "Classic99 Error", MB_OK);
			if (lpdd) lpdd->Release();
			lpdd=NULL;
			StretchMode=0;
			goto optout;
		}

		ZeroMemory(&CurrentDDSD, sizeof(CurrentDDSD));
		CurrentDDSD.dwSize=sizeof(CurrentDDSD);
		CurrentDDSD.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
		switch (FilterMode) {
			case 0:		// none
				CurrentDDSD.dwWidth=256+16;
				CurrentDDSD.dwHeight=192+16;
				break;

			case 4:		// TV
				CurrentDDSD.dwWidth=TV_WIDTH;
				CurrentDDSD.dwHeight=384+29;
				break;

			case 5:		// hq4x
				CurrentDDSD.dwWidth=(256+16)*4;
				CurrentDDSD.dwHeight=(192+16)*4;
				break;

			default:	// others (*2)
				CurrentDDSD.dwWidth=512+32;
				CurrentDDSD.dwHeight=384+29;
				break;
		}

		if (lpdd->CreateSurface(&CurrentDDSD, &ddsBack, NULL) !=DD_OK) {
			MessageBox(myWnd, "Unable to create back buffer surface\nDX mode is not available", "Classic99 Error", MB_OK);
			ddsBack=NULL;
			lpdds->Release();
			lpdds=NULL;
			lpdd->Release();
			lpdd=NULL;
			StretchMode=0;
			goto optout;
		}

		if (!fullscreen) {
			if (lpdd->CreateClipper(0, &lpDDClipper, NULL) != DD_OK) {
				MessageBox(myWnd, "Warning: Unable to create Direct Draw Clipper", "Classic99 Warning", MB_OK);
			} else {
				if (lpDDClipper->SetHWnd(0, myWnd) != DD_OK) {
					MessageBox(myWnd, "Warning: Unable to set Clipper Window", "Classic99 Warning", MB_OK);
					lpDDClipper->Release();
					lpDDClipper=NULL;
				} else {
					if (DD_OK != lpdds->SetClipper(lpDDClipper)) {
						MessageBox(myWnd, "Warning: Unable to attach Clipper", "Classic99 Warning", MB_OK);
						lpDDClipper->Release();
						lpDDClipper=NULL;
					}
				}
			}
		}
	}
	LeaveCriticalSection(&VideoCS);
	return;

optout: ;
	takedownDirectDraw();
	LeaveCriticalSection(&VideoCS);
}

////////////////////////////////////////////////////////////
// Release all references to DirectDraw objects
////////////////////////////////////////////////////////////
void takedownDirectDraw() {	
	EnterCriticalSection(&VideoCS);

	if (NULL != lpDDClipper) lpDDClipper->Release();
	lpDDClipper=NULL;
	if (NULL != ddsBack) ddsBack->Release();
	ddsBack=NULL;
	if (NULL != lpdds) lpdds->Release();
	lpdds=NULL;
	if (NULL != lpdd) lpdd->Release();
	lpdd=NULL;

	LeaveCriticalSection(&VideoCS);
}

////////////////////////////////////////////////////////////
// Resize the back buffer
////////////////////////////////////////////////////////////
int ResizeBackBuffer(int w, int h) {
	EnterCriticalSection(&VideoCS);

	if (NULL != ddsBack) ddsBack->Release();
	ddsBack=NULL;

	if (NULL == lpdd) {
		SetupDirectDraw(0);
		if (NULL == lpdd) {
			MessageBox(myWnd, "Unable to create back buffer surface\nDX mode is not available", "Classic99 Error", MB_OK);
			ddsBack=NULL;
			StretchMode=0;
			LeaveCriticalSection(&VideoCS);
			return 1;
		}
	}

	ZeroMemory(&CurrentDDSD, sizeof(CurrentDDSD));
	CurrentDDSD.dwSize=sizeof(CurrentDDSD);
	CurrentDDSD.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
	CurrentDDSD.dwWidth=w;
	CurrentDDSD.dwHeight=h;

	if (lpdd->CreateSurface(&CurrentDDSD, &ddsBack, NULL) != DD_OK) {
		MessageBox(myWnd, "Unable to create back buffer surface\nDX mode is not available", "Classic99 Error", MB_OK);
		ddsBack=NULL;
		StretchMode=0;
		LeaveCriticalSection(&VideoCS);
		return 1;
	}

	LeaveCriticalSection(&VideoCS);
	return 0;
}

//////////////////////////////////////
// Save a screenshot - just BMP for now
// there are lots of nice helpers for others in
// 2000 and higher, but that's ok 
//////////////////////////////////////
void SaveScreenshot(bool bAuto, bool bFiltered) {
	static int nLastNum=0;
	static CString csFile;
	CString csTmp;
	OPENFILENAME ofn;
	char buf[256], buf2[256];
	BOOL bRet;

	if ((!bAuto) || (csFile.IsEmpty())) {
		memset(&ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize=sizeof(OPENFILENAME);
		ofn.hwndOwner=myWnd;
		ofn.lpstrFilter="BMP Files\0*.bmp\0\0";
		strcpy(buf, "");
		ofn.lpstrFile=buf;
		ofn.nMaxFile=256;
		strcpy(buf2, "");
		ofn.lpstrFileTitle=buf2;
		ofn.nMaxFileTitle=256;
		ofn.Flags=OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

		char szTmpDir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, szTmpDir);

		bRet = GetSaveFileName(&ofn);

		SetCurrentDirectory(szTmpDir);

		csTmp = ofn.lpstrFile;				// save the file we are opening now
		if (ofn.nFileExtension > 1) {
			csFile = csTmp.Left(ofn.nFileExtension-1);
		} else {
			csFile = csTmp;
			csTmp+=".bmp";
		}
	} else {
		int nCnt=10000;
		for (;;) {
			csTmp.Format("%s%04d.bmp", (LPCSTR)csFile, nLastNum++);
			FILE *fp=fopen(csTmp, "r");
			if (NULL != fp) {
				fclose(fp);
				nCnt--;
				if (nCnt == 0) {
					MessageBox(myWnd, "Can't take another auto screenshot without overwriting file!", "Classic99 Error", MB_OK);
					return;
				}
				continue;
			}
			break;
		}
	}

	if (bRet) {
		// we just create a 24-bit BMP file
		int nX, nY, nBits;
		unsigned char *pBuf;

		if (bFiltered) {
			switch (FilterMode) {
			case 0:		// none
				nX=256+16;
				nY=192+16;
				pBuf=(unsigned char*)framedata;
				nBits=32;
				break;
			
			case 4:		// TV
				nX=TV_WIDTH;
				nY=384+29;
				pBuf=(unsigned char*)framedata2;
				nBits=32;
				break;

			case 5:		// hq4x
				nX=(256+16)*4;
				nY=(192+16)*4;
				pBuf=(unsigned char*)framedata2;
				nBits=32;
				break;

			default:	// All SAI2x modes
				nX=512+32;
				nY=384+29;
				pBuf=(unsigned char*)framedata2;
				nBits=32;
				break;
			}
		} else {
			nX=256+16;
			nY=192+16;
			pBuf=(unsigned char*)framedata;
			nBits=32;
		}

		FILE *fp=fopen(csTmp, "wb");
		if (NULL == fp) {
			MessageBox(myWnd, "Failed to open output file", "Classic99 Error", MB_OK);
			return;
		}

		int tmp;
		fputc('B', fp);				// signature, BM
		fputc('M', fp);
		tmp=nX*nY*3+54;
		fwrite(&tmp, 4, 1, fp);		// size of file
		tmp=0;
		fwrite(&tmp, 4, 1, fp);		// four reserved bytes (2x 2 byte fields)
		tmp=26;
		fwrite(&tmp, 4, 1, fp);		// offset to data
		tmp=12;
		fwrite(&tmp, 4, 1, fp);		// size of the header (v1)
		fwrite(&nX, 2, 1, fp);		// width in pixels
		fwrite(&nY, 2, 1, fp);		// height in pixels
		tmp=1;
		fwrite(&tmp, 2, 1, fp);		// number of planes (1)
		tmp=24;
		fwrite(&tmp, 2, 1, fp);		// bits per pixel (0x18=24)

		if (nBits == 16) {
			// 16-bit 0rrr rrgg gggb bbbb values
			// TODO: not used anymore
			unsigned short *p = (unsigned short*)pBuf;

			for (int idx=0; idx<nX*nY; idx++) {
				int r,g,b;
				
				// extract colors
				r=((*p)&0x7c00)>>10;
				g=((*p)&0x3e0)>>5;
				b=((*p)&0x1f);

				// scale up from 5 bit to 8 bit
				r<<=3;
				g<<=3;
				b<<=3;

				// write out to file
				fputc(b, fp);
				fputc(g, fp);
				fputc(r, fp);

				p++;
			}
		} else {
			// 32-bit 0BGR
			for (int idx=0; idx<nX*nY; idx++) {
				int r,g,b;
				
				// extract colors
				b=*pBuf++;
				g=*pBuf++;
				r=*pBuf++;
				pBuf++;					// skip	0

				// write out to file
				fputc(b, fp);
				fputc(g, fp);
				fputc(r, fp);
			}
		}

		fclose(fp);

		CString csTmp2;
		csTmp2.Format("Classic99 - Saved %sfiltered - %s", bFiltered?"":"un", (LPCSTR)csTmp);
		SetWindowText(myWnd, csTmp2);
	}
}

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

// captures a text or graphics mode screen (will do bitmap too, assuming graphics mode)
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


