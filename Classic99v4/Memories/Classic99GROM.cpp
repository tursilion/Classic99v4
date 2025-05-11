// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This class emulates GROM devices for the TI - it's not a full peripheral itself
// and is meant to be encapsulated in a class that defines the data.
// See header file for lots of notes.

#include "Classic99GROM.h"

// static data storage - todo: bases?
// because the GROMs maintain their own address counter, we need all the
// GROM sources to load to a common data target
uint8_t Classic99GROM::GROMDATA[64*1024] = { 0 };
int Classic99GROM::GRMADD = 0;
int Classic99GROM::grmaccess = 0;
int Classic99GROM::grmdata = 0;
int Classic99GROM::LastRead = 0; 
int Classic99GROM::LastBase = 0;

// not much needed for construction, except pointing to the data
Classic99GROM::Classic99GROM(Classic99System *core, const uint8_t *pDat, unsigned int datSize, unsigned int baseAddress) 
    : Classic99Peripheral(core)
//    , grmBase(0)
{
	GRMADD=0;
    grmaccess=2;
	grmdata=0;
    LastRead=0;
    LastBase=0;

    // all GROMs are read-only (ie: not GRAM)
    // TODO: or should I make GRAM a separate device altogether???
    // probably yes...
    for (int idx=0; idx<8; ++idx) {
        bWritable[idx] = false;
    }

    if (NULL == pDat) {
        debug_write("NULL pointer to GROM init, failed to load.");
    } else if (baseAddress + datSize > 64*1024) {
        debug_write("GROM memory overrun, failed to load.");
    } else {
        memcpy(GROMDATA+baseAddress, pDat, datSize);
    }
}
Classic99GROM::~Classic99GROM() {
}

// protected methods to let derived classes load additional data without a new object
// really just a hack for demonstration, normally create a new object
void Classic99GROM::LoadAdditionalData(const uint8_t *pDat, unsigned int datSize, unsigned int baseAddress) {
    if (NULL == pDat) {
        debug_write("NULL pointer to GROM init, failed to load.");
    } else if (baseAddress + datSize > 64*1024) {
        debug_write("GROM memory overrun, failed to load.");
    } else {
        memcpy(GROMDATA+baseAddress, pDat, datSize);
    }
}

// increment the GROM address and handle wraparound
void Classic99GROM::IncrementGROMAddress(int &adrRef) {
    int base = adrRef&0xe000;
    adrRef = ((adrRef+1)&0x1fff) | base;
}

uint8_t Classic99GROM::read(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw) {
    // since we are a memory mapped device, the meaning of addr varies
    // however, we should be mapped such that we don't need to think too hard about it at this
    // point, the appropriate addresses should be mapped to the appropriate objects, so all
    // we really care is whether the read is for address or data
    (void)isIO;

    if (addr & GROM_MODE_WRITE) {
        // no additional cycles needed
        return 0;
    }

	// NOTE: Classic99 does not emulate the 6k GROM behaviour that causes mixed
	// data in the range between 6k and 8k - because Classic99 assumes all GROMS
	// are actually 8k devices. It will return whatever was in the ROM data it
	// loaded (or zeros if no data was loaded there). I don't intend to reproduce
	// this behaviour (but I can certainly conceive of using it for copy protection,
	// if only real GROMs could still be manufactured...)
    // TODO: actually, we can easily incorporate it by generating the extra 2k when we
    // load a 6k grom... but it would only work if we loaded 6k groms instead of
    // the combined banks lots of people use. But at least we'd do it.

    // any valid read resets the access flag
    grmaccess=2;

	if (addr & GROM_MODE_ADDRESS) {
        // address read is destructive
		uint8_t z=(GRMADD&0xff00)>>8;
		GRMADD=(((GRMADD&0xff)<<8)|(GRMADD&0xff));
		// TODO: Is the address incremented anyway? ie: if you keep reading, what do you get?

        // GROM read address always adds about 13 cycles
		if (rmw != ACCESS_FREE) cycles += 13;
		return(z);
	} else {
		// reading data

		//UpdateHeatGROM(GRMADD);

        // this saves some debug off for Rich
        LastRead = GRMADD;
//        LastBase = grmBase;       // TODO: GROM bases
        LastBase = 0;

        // store data, and update prefetch
		uint8_t z=grmdata;
		grmdata=GROMDATA[GRMADD];
        IncrementGROMAddress(GRMADD);

        // GROM read data always adds about 19 cycles
		if (rmw != ACCESS_FREE) cycles += 19;
		return(z);
    }
}

void Classic99GROM::write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) {
    (void)isIO;

    if (0 == (addr&GROM_MODE_WRITE)) {
        // writing to read address is ignored
        return;
    }

//	if (nBase > 0) {
//		debug_write("Write GROM base %d(>%04X), >%04x, >%02x, %d", nBase, x, GROMBase[0].GRMADD, c, GROMBase[0].grmaccess);
//	}

	// Note that UberGROM access time (in the pre-release version) was 15 cycles (14.6), but it
	// does not apply as long as other GROMs are in the system (and they have to be due to lack
	// of address counter.) So this is still valid.

	if (addr&GROM_MODE_ADDRESS) {
		// write GROM address
		GRMADD=((GRMADD<<8)|(data))&0xffff;
		grmaccess--;
		if (grmaccess==0) {
			// time to do a prefetch
			grmaccess=2;

            // second GROM address write adds about 21 cycles (verified)
    		if (rmw != ACCESS_FREE) cycles += 21;
			
			// update prefetch
			grmdata=GROMDATA[GRMADD];
            IncrementGROMAddress(GRMADD);
		} else {
            // first GROM address write adds about 15 cycles (verified)
    		if (rmw != ACCESS_FREE) cycles += 15;
        }
	}
	else
	{
		// GROM writes do not directly affect the prefetches, but have the same
		// side effects as reads (they increment the address and perform a
		// new prefetch)

		//UpdateHeatGROM(GRMADD);

        // like reads, reset the access counter
		grmaccess=2;

		// Since all GRAM devices were hacks, they apparently didn't handle prefetch the same
		// way as I expected. Because of prefetch, the write address goes to the GROM address
		// minus one. Well, they were hacks in hardware, I'll just do a hack here.

		int nRealAddress = (GRMADD-1)&0xffff;
		if (bWritable[(nRealAddress&0xE000)>>13]) {
			// Allow it! The user is crazy! :)
			GROMDATA[nRealAddress]=data;
		}

        // update prefetch
    	grmdata=GROMDATA[GRMADD];
   		IncrementGROMAddress(GRMADD);

        // GROM data writes add about 22 cycles (verified)
   		if (rmw != ACCESS_FREE) cycles += 22;
	}

}

bool Classic99GROM::init(int idx) {
    // nothing special to do here
    setIndex("GROM", idx);
    return true;
}

bool Classic99GROM::operate(double timestamp) {
    // nothing special to do here
    return true;
}

bool Classic99GROM::cleanup() {
    // nothing special to do here
    return true;
}

void Classic99GROM::getDebugSize(int &x, int &y, int user) {
    // not sure at the moment how this is going to work... might need to add
    // a view method as well as a debug window... or at least a variable offset
    // We need a way to view the memory bytes, maybe edit them, as well as to
    // view the various address counters. TODO.
    x=0;
    y=0;
}

void Classic99GROM::getDebugWindow(char *buffer, int user) {
    // nothing at the moment
}

int Classic99GROM::saveStateSize() {
    return 8+64*1024+4+4+4+4+4;
}

bool Classic99GROM::saveState(unsigned char *buffer) {
    for (int idx=0; idx<8; ++idx) {
        *(buffer++) = bWritable[idx] ? 1 : 0;
    }
    memcpy(buffer, GROMDATA, 64*1024);
    buffer+=64*1024;

    saveStateVal(buffer, GRMADD);
    saveStateVal(buffer, grmaccess);
    saveStateVal(buffer, grmdata);
    saveStateVal(buffer, LastRead);
    saveStateVal(buffer, LastBase);

    return true;
}

bool Classic99GROM::restoreState(unsigned char *buffer) {
    for (int idx=0; idx<8; ++idx) {
        bWritable[idx] = *(buffer++) ? true : false;
    }

    memcpy(GROMDATA, buffer, 64*1024);
    buffer+=64*1024;

    loadStateVal(buffer, GRMADD);
    loadStateVal(buffer, grmaccess);
    loadStateVal(buffer, grmdata);
    loadStateVal(buffer, LastRead);
    loadStateVal(buffer, LastBase);

    return true;
}

void Classic99GROM::testBreakpoint(bool isRead, int addr, bool isIO, int data) {
    (void)isIO;

    // call the base class for the raw address test
    Classic99Peripheral::testBreakpoint(isRead, addr, isIO, data);

    // now do similar, but based on the GROM address instead
    // assumes a non-sparse breaks[] array!
    for (int idx=0; idx<MAX_BREAKPOINTS; ++idx) {
        if (breaks[idx].typeMask == 0) break;
            if ((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_GROM) == 0) continue;             // check is for GROM
            if (!breaks[idx].CheckRange(GRMADD, 0)) continue;                                       // check address match (most likely false)
            if ((isRead)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_READ)==0)) continue;   // check for read
            if ((!isRead)&&((breaks[idx].typeMask&BREAKPOINT::BREAKPOINT_MASK_WRITE)==0)) continue; // check for write
            // we've narrowed it down a lot at this point, check the less likely things...
            if (breaks[idx].page != page) continue;

            // TODO: add ignore console breakpoints (meaning ALL breakpoints need to check the primary CPU address,
            // which is in InterestingData, and we need a range check on the system core to determine ROM addresses...
            // maybe this can also be in InterestingData, along with the ignore rom hits flag.)

            // So we have confirmed whether it's read or write, IO or not, all that is left
            // is to compare the data (and a mere access breakpoint will have a zero mask)
            if ((breaks[idx].dataMask != 0) && ((data&breaks[idx].dataMask) != (breaks[idx].data&breaks[idx].dataMask))) continue;

            theCore->triggerBreakpoint();
            break;
    }
}
