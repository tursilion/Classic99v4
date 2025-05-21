/*****************************************************************************
 *
 *	 9900dasm.c
 *	 TMS 9900 disassembler
 *
 *	 Copyright (c) 1998 John Butler, all rights reserved.
 *	 Based on 6502dasm.c 6502/65c02/6510 disassembler by Juergen Buchmueller
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/
// Modified by Tursi for Classic99 

void ImportMapFile(const char *fn);
int Dasm9900(char *buffer, unsigned short *bufferIn, bool isF18A, bool enableDebugOpcodes);

// Maybe not the best place for this, but I need to put it somewhere
// Size of backtrace for CPUs using this
#define RUNLOGSIZE 64
#define DISASMWIDTH 80

// disassembly log - we'll keep a history of 50 instructions
typedef struct {
    int pc;
    //int bank;                 // hmm. CPU doesn't know anything about memory banks... TODO
    unsigned short words[4];
    int lines;                  // number of lines in output text, if already processed (target specific)
    char str[DISASMWIDTH*3];    // 3 fixed width 80 character lines
} ByteHistory;

