// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// TODO: someday - also render the emulator window in a pane. Then we can have a startup option
// to not initialize graphics and audio, and run entirely in an SSH window. That would be pretty fun.
// TODO: an option to close the console (maybe with a keypress to reopen it?), or at least push it behind the main window
// TODO: future: multiple consoles for multiple debug windows
// TODO: maybe color someday?

#include "os.h"
#include <raylib.h>
#include <ncurses.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include "automutex.h"
#include "debuglog.h"

// size of debug log in memory - len is characters, lines is lines
#define DEBUGLEN 1024
#define DEBUGLINES 50
static char lines[DEBUGLINES][DEBUGLEN];
bool bDebugDirty = false;
int currentDebugLine;
std::recursive_mutex *debugLock;      // our object lock - MUST HOLD FOR ALL NCURSES ACTIVITY, in case it's not thread safe
std::thread *debugThread;   // the actual thread object

using namespace std::chrono_literals;

// TODO: someday this library may help us go to UTF8: https://github.com/neacsum/utf8

// TODO: I'll probably create some kind of class to display all the debug windows, including this...

void debug_thread_init() {
    autoMutex mutex(debugLock);

    // curses setup
    initscr();
    raw();                  // no signal keypresses
    noecho();               // no character echo
    keypad(stdscr, TRUE);   // enable extended keys
    nodelay(stdscr, TRUE);  // non-blocking getch

    // our own setup
    memset(lines, 0, sizeof(lines));
    bDebugDirty = true;
}

// the main loop that manages the debug system - debugLock must be created BEFORE calling
void debug_thread() {
    bool quit = false;
    threadname("debug_thread");

    // initialization
    debug_thread_init();

    while (!quit) {
        int ch;
        {
            autoMutex mutex(debugLock);
            ch = getch();

            if (ch != ERR) {
                debug_write("Got char: %c\n", ch);
                if (ch =='Q') quit=true;    //TODO: obviously I don't want to quit on 'Q'
            }
        }

        // sleep 5ms or whatever quantum is, it's only keypresses
        std::this_thread::sleep_for(5ms);
    }

    // If the loop above exits, then tell the emulator to shut down via Raylib
    RequestClose();
}

//--- functions above this point are intended to be used by the debug_thread() only
//--- functions below this point are intended to be called from anywhere in the application

void debug_init() {
    debugLock = new std::recursive_mutex();

    // Start the debug thread
    debugThread = new std::thread(debug_thread);
    // give the thread ample time to start
    std::this_thread::sleep_for(100ms);

    // try to acquire the lock - that will tell us it's finished
    {
        autoMutex mutex(debugLock);
    }

    debug_write("Debug thread initialized");
}
void debug_shutdown() {
    delete debugLock;
    endwin();
}

// Write a line to the debug buffer displayed on the debug screen
void debug_write(const char *s, ...)
{
    char buf[DEBUGLEN];

    // full length debug output
    va_list argptr;
    va_start(argptr, s);
    vsnprintf(buf, DEBUGLEN-1, s, argptr);
    buf[DEBUGLEN-1]='\0';

#ifdef _WINDOWS
    // output to Windows debug listing...
    OutputDebugString(buf);
    OutputDebugString("\n");
#endif

    // truncate to rolling array size
    buf[DEBUGLEN-1]='\0';

    {
        autoMutex lock(debugLock);

        memset(&lines[currentDebugLine][0], ' ', DEBUGLEN);	// clear line
        strncpy(&lines[currentDebugLine][0], buf, DEBUGLEN);    // copy in new line
        lines[currentDebugLine][DEBUGLEN-1]='\0';               // zero terminate
        if (++currentDebugLine >= DEBUGLINES) currentDebugLine = 0;

    	bDebugDirty=true;									    // flag redraw

        printw("%s\n", buf);
        refresh();
    }

}

// size of the debug text output
void debug_size(int &x, int &y) {
    x = DEBUGLEN+2; // assumes each line includes \r\n on output
    y = DEBUGLINES;
}

// fill in a text buffer
void fetch_debug(char *buf) {
    // buf MUST be debug_size() x*y bytes long
    memset(buf, ' ', DEBUGLINES*(DEBUGLEN+2));

    {
        autoMutex mutex(debugLock);

        int line = currentDebugLine;
        for (int idx=0; idx<DEBUGLINES; ++idx) {
            sprintf(buf, "%s\r\n", lines[line++]);
            buf += DEBUGLEN+2;
        }
    }
}

