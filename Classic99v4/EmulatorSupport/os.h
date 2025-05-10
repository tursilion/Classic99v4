// Classic99 v4xx - Copyright 2025 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// Just a dumping ground for anything I need to abstract across the OS's that the
// other libraries don't already do.

#if defined(__APPLE__)
#include <pthread.h>

#elif defined(_WINDOWS)
// a few names conflict with Raylib and PDCurses
#define Rectangle WinRectangle
#define CloseWindow WinCloseWindow
#define ShowCursor WinShowCursor
#define LoadImageA WinLoadImageA
#define DrawTextA WinDrawTextA
#define DrawTextExA WinDrawTextExA
#define PlaySoundA WinPlaySoundA
#include <Windows.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImageA
#undef DrawTextA
#undef DrawTextExA
#undef PlaySoundA
// windows declared these
#undef MOUSE_MOVED

#else
#include <sys/prctl.h>

#endif

#ifndef EMULATOR_SUPPORT_OS_H
#define EMULATOR_SUPPORT_OS_H

// set thread name for debugging
#if defined(__APPLE__)
#define threadname(X) pthread_setname_np(X)

#elif defined(_WINDOWS)
#define threadname(X) {                                     \
    size_t newsize = strlen(X) + 1;                         \
    wchar_t * wcstring = new wchar_t[newsize];              \
    size_t convchar = 0;                                    \
    mbstowcs_s(&convchar, wcstring, newsize, X, _TRUNCATE); \
    SetThreadDescription(GetCurrentThread(), wcstring);     \
    delete[] wcstring;                                      \
}

#else
#define threadname(X) prctl(PR_SET_NAME, X, 0, 0, 0);

#endif


#endif
