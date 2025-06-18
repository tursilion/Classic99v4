// Classic99 v4xx - Copyright 2025 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// Implements a curses based debugger in the console window
// always did like text mode

#ifndef EMULATOR_SUPPORT_DEBUGMENU_H
#define EMULATOR_SUPPORT_DEBUGMENU_H

#include <ncurses.h>

class Classic99Peripheral;
class MenuTrack;

// top level menus - numbers can be reused by different subsystems
// some of these are also just commands to debug that don't come from menus
enum {
    DEBUG_CMD_FILE_COLDRESET = 0x7000,
    DEBUG_CMD_FILE_QUIT,
    DEBUG_CMD_EDIT_SPLIT_DEBUG,
    DEBUG_CMD_EDIT_COLLAPSE_DEBUG,
    DEBUG_CMD_RESET_TI994,
    DEBUG_CMD_RESET_TI994A,
    DEBUG_CMD_RESET_TI994A22,
    DEBUG_CMD_CART_APPS,
    DEBUG_CMD_CART_GAMES,
    DEBUG_CMD_CART_OPEN,
    DEBUG_CMD_CART_EJECT,
    DEBUG_CMD_PERIPHERAL_OPEN,
    DEBUG_CMD_HELP_ABOUT,
    DEBUG_CMD_HELP_KEYS,
    DEBUG_CMD_FORCE_DEBUG,
    DEBUG_CMD_FORCE_VDP
};

// small structure for initializing menus
struct MenuInit {
    const char *str;
    int cmd;
};

// small class for internally tracking menus
class MenuTrack {
public:
    MenuTrack(Classic99Peripheral *p, MenuInit *m);
    MenuTrack() = delete;
    ~MenuTrack();
    void drawMenu(int r, int c);
    int getCurrentKey();
    int getMenuCnt() { return menuCnt; }
    const char* getMenuName() { return pMenu[0].str; }

    Classic99Peripheral *pOwner;    // warning: can be NULL!
    int currentSelection;           // what index is the user on?

private:
    struct MenuInit *pMenu;
    int menuCnt;
    int maxWidth;
};

MenuTrack *debug_add_menu(MenuInit *menuList);
MenuTrack *debug_add_peripheral_menu(Classic99Peripheral *pOwner, MenuInit *menuList);
void debug_unregister_menu(Classic99Peripheral *pOwner);
void menu_open_first();
bool menu_update(int ch);
void debug_open_menu(MenuTrack *pMenu);
void debug_close_menu();
void debug_close_all_menu();
void debug_init_menu();
void debug_deinit_menu();

#endif

