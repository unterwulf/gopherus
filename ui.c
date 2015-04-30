/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on screen.
 *
 * Copyright (C) Mateusz Viste 2013
 *
 * Provides all UI functions used by Gopherus, basing on DJGPP conio facilities.
 */

#include <conio.h>
#include <pc.h>    /* ScreenRows() */
#include "ui.h"

int ui_getrowcount()
{
    return ScreenRows();
}

int ui_getcolcount()
{
    return ScreenCols();
}

void ui_cls()
{
    clrscr();
}

void ui_puts(char *str)
{
    cprintf("%s\r\n", str);
}

void ui_locate(int y, int x)
{
    ScreenSetCursor(y, x);
}

void ui_putchar(char c, int attr, int x, int y)
{
    ScreenPutChar(c, attr, x, y);
}

int ui_getkey()
{
    return getkey();
}

int ui_kbhit()
{
    return kbhit();
}

void ui_cursor_show()
{
    _setcursortype(_NORMALCURSOR);
}

void ui_cursor_hide()
{
    _setcursortype(_NOCURSOR);
}
