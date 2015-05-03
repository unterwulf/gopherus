/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on terminal's screen.
 *
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef UI_H
#define UI_H

void ui_init(void);

/* returns the number of rows of current text mode */
int ui_getrowcount(void);

/* returns the number of columns of current text mode */
int ui_getcolcount(void);

/* clear the screen */
void ui_cls(void);

/* print a string on screen, and go to next line */
void ui_puts(char *str);

void ui_cputs(const char *str, int attr, int x, int y);

/* Set the position (zero-based) of the cursor on screen */
void ui_locate(int x, int y);

/* Put a char directly on screen, without playing with the cursor. Coordinates are zero-based. */
void ui_putchar(char c, int attr, int x, int y);

/* waits for a key to be pressed and returns it. ALT+keys have 0x100 added to them. */
int ui_getkey(void);

/* returns 0 if no key is awaiting in the keyboard buffer, non-zero otherwise */
int ui_kbhit(void);

/* makes the cursor visible */
void ui_cursor_show(void);

/* hides the cursor */
void ui_cursor_hide(void);

#endif
