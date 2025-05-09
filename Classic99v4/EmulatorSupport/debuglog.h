// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// Implements a curses based debugger in the console window
// always did like text mode

#ifndef EMULATOR_SUPPORT_DEBUGLOG_H
#define EMULATOR_SUPPORT_DEBUGLOG_H

void debug_init();
void debug_shutdown();
void debug_write(const char *s, ...);
void debug_size(int &x, int &y);
void fetch_debug(char *buf);

#endif

