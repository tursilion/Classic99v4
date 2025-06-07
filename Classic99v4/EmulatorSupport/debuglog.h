// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// Implements a curses based debugger in the console window
// always did like text mode

#ifndef EMULATOR_SUPPORT_DEBUGLOG_H
#define EMULATOR_SUPPORT_DEBUGLOG_H

#include <ncurses.h>

class Classic99Peripheral;
class WindowTrack;

void debug_init();
void debug_stop();
void debug_shutdown();
void debug_write(const char *s, ...);
void debug_write_var(const char *s, va_list argptr);
void debug_size(int &x, int &y, int nuser);
void fetch_debug(char *buf, int nuser);
void debug_control(int command);

WindowTrack *debug_create_view(Classic99Peripheral *pOwner, int user);
void debug_unregister_view(Classic99Peripheral *pOwner);

// small class for internally tracking tui windows
class WindowTrack {
public:
    WindowTrack(Classic99Peripheral *p, int user);
    WindowTrack() = delete;
    ~WindowTrack();

    int minr, minc;
    int userval;
    Classic99Peripheral *pOwner;    // warning: can be NULL!
    char szname[16];
};

#endif

