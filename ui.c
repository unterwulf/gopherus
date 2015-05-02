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

void ui_init(void)
{
    return;
}

int ui_getrowcount(void)
{
    return ScreenRows();
}

int ui_getcolcount(void)
{
    return ScreenCols();
}

void ui_cls(void)
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

int ui_getkey(void)
{
    return getkey();
}

int ui_kbhit(void)
{
    return kbhit();
}

void ui_cursor_show(void)
{
    _setcursortype(_NORMALCURSOR);
}

void ui_cursor_hide(void)
{
    _setcursortype(_NOCURSOR);
}
