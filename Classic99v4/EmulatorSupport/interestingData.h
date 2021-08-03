// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef EMULATOR_SUPPORT_INTERESTINGDATA_H
#define EMULATOR_SUPPORT_INTERESTINGDATA_H

// system types for the interesting data
enum {
    SYSTEM_TI99
};

// The design of the system assumes that each CPU type will have no more than one instantiation.
// For the machines I intend to emulate, that's sufficient, so each CPU type can have its own flag set.
enum {
// System data
    DATA_SYSTEM_TYPE,                       // TODO

// Emulator data
    DATA_BREAK_ON_ILLEGAL,                  // TODO
    DATA_CHECK_UNINITIALIZED_MEMORY,        // TODO
    DATA_BREAK_ON_DISK_CORRUPT,             // TODO

// Specific chip data (only what's needed)
    DATA_TMS9900_PC,                        // TODO - used by hacks and disk system
    DATA_TMS9900_WP,                        // TODO - used by hacks and disk system
    DATA_TMS9900_INTERRUPTS_ENABLED,

// Special case debug data - may not be set in all cases
    DATA_TMS9900_8356,                      // contents of TMS9900 memory word >8356, special to TI disk emulation
    DATA_TMS9900_8370,                      // contents of TMS9900 memory word >8356, special to TI disk emulation

// Indirect data - must use get Indirect version
    INDIRECT_MAIN_CPU_INTERRUPTS_ENABLED,   // this will point to the correct index for the main CPU's interrupt enabled flag
    INDIRECT_MAIN_CPU_PC,                   // this will point to the correct index for the main CPU's PC
                                        
// final value for size
    MAX_INTERESTING_DATA
};

// using my own here just to be sure the values are compatible
// note that full ints are valid for many datas
constexpr int DATA_TRUE = 1;
constexpr int DATA_FALSE = 0;
constexpr int DATA_UNSET = -1;

extern int InterestingData[MAX_INTERESTING_DATA];

// TOOD: call during system initialization to reset the database
void initInterestingData();

inline bool hasInterestingData(int type) {
    if (type < MAX_INTERESTING_DATA) {
        return (InterestingData[type] != DATA_UNSET);
    } else {
        return false;
    }
}
inline int getInterestingData(int type) {
    if (type < MAX_INTERESTING_DATA) {
        return InterestingData[type];
    } else {
        return DATA_UNSET;
    }
}
inline void setInterestingData(int type, int value) {
    if (type < MAX_INTERESTING_DATA) {
        InterestingData[type] = value;
    }
}
inline int getInterestingDataIndirect(int type) {
    if (type < MAX_INTERESTING_DATA) {
        int x = InterestingData[type];
        if ((x < MAX_INTERESTING_DATA)&&(x != DATA_UNSET)) {
            return InterestingData[x];
        }
    }
    
    return DATA_UNSET;
}

#endif

