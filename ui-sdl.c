/*
 * This file is part of the gopherus project.
 * It provides abstract functions to draw on screen.
 *
 * Copyright (C) Mateusz Viste 2013
 *
 * Provides all UI functions used by Gopherus, wrapped around a virtual terminal emulated via SDL calls.
 */

#include <SDL/SDL.h>
#include "common.h"
#include "ui.h"
#include "ascii.h" /* ascii fonts */

static int cursorx, cursory;
static SDL_Surface *screen;
static int cursorstate = 1;

/* This function has been borrowed from the SDL documentation */
static void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            *p = pixel;
            break;
        case 2:
            *(Uint16 *)p = pixel;
            break;
        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;
        case 4:
            *(Uint32 *)p = pixel;
            break;
    }
}

void ui_init(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    screen = SDL_SetVideoMode(640, 480, 32, 0);
    SDL_WM_SetCaption("Gopherus", NULL);
    SDL_EnableKeyRepeat(800, 80); /* enable repeating keys */
    SDL_EnableUNICODE(1);  /* using the SDL unicode support actually for getting ASCII */
    atexit(SDL_Quit); /* clean up at exit time */
}

int ui_getrowcount(void)
{
    return 30;
}

int ui_getcolcount(void)
{
    return 80;
}

void ui_cls(void)
{
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);
}

void ui_puts(char *str)
{
    puts(str);
}

void ui_locate(int x, int y)
{
    cursorx = x;
    cursory = y;
}

void ui_putchar(char c, int attr, int x, int y)
{
    int xx, yy;
    const long attrpal[16] = {0x000000l, 0x0000AAl, 0x00AA00l, 0x00AAAAl, 0xAA0000l, 0xAA00AAl, 0xAA5500l, 0xAAAAAAl, 0x555555l, 0x5555FFl, 0x55FF55l, 0x55FFFFl, 0xFF5555l, 0xFF55FFl, 0xFFFF55l, 0xFFFFFFl};

    for (yy = 0; yy < 16; yy++) {
        for (xx = 0; xx < 8; xx++) {
            if ((ascii_font[((unsigned)c << 4) + yy] & (1 << xx)) != 0) {
                putpixel(screen, (x << 3) + 7 - xx, (y << 4) + yy, attrpal[attr & 0x0F]);
            } else {
                putpixel(screen, (x << 3) + 7 - xx, (y << 4) + yy, attrpal[attr >> 4]);
            }
        }
    }

    /* draw a cursor over if cursor is enabled and position is right.. this is really clumsy, but it works for now... */
    if ((cursorstate != 0) && (cursorx == x) && (cursory == y))
        for (yy = 0; yy < 16; yy++)
            for (xx = 0; xx < 8; xx++)
                if ((ascii_font[('_' << 4) + yy] & (1 << xx)) != 0)
                    putpixel(screen, (x << 3) + 7 - xx, (y << 4) + yy, attrpal[attr & 0x0F]);

    /* tell SDL to update the character's area on screen */
    SDL_UpdateRect(screen, (x << 3), (y << 4), 8, 16);
}

int ui_getkey(void)
{
    SDL_Event event;

    for (;;) {
        if (SDL_WaitEvent(&event) == 0)
            return 0; /* block until an event is received */

        if (event.type == SDL_KEYDOWN) { /* I'm only interested in key presses */
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    return KEY_ESCAPE;
                case SDLK_BACKSPACE:
                    return KEY_BACKSPACE;
                case SDLK_TAB:
                    return KEY_TAB;
                case SDLK_RETURN:
                    return KEY_ENTER;
                case SDLK_F1:
                    return KEY_F1;
                case SDLK_F2:
                    return 0x13C;
                case SDLK_F3:
                    return 0x13D;
                case SDLK_F4:
                    return 0x13E;
                case SDLK_F5:
                    return KEY_F5;
                case SDLK_F6:
                    return 0x140;
                case SDLK_F7:
                    return 0x141;
                case SDLK_F8:
                    return 0x142;
                case SDLK_F9:
                    return KEY_F9;
                case SDLK_F10:
                    return 0x144;
                case SDLK_HOME:
                    return KEY_HOME;
                case SDLK_UP:
                    return KEY_UP;
                case SDLK_PAGEUP:
                    return KEY_PAGEUP;
                case SDLK_LEFT:
                    return KEY_LEFT;
                case SDLK_RIGHT:
                    return KEY_RIGHT;
                case SDLK_END:
                    return KEY_END;
                case SDLK_DOWN:
                    return KEY_DOWN;
                case SDLK_PAGEDOWN:
                    return KEY_PAGEDOWN;
                case SDLK_DELETE:
                    return KEY_DELETE;
                default: /* ignore anything else, unless it's classic ascii */
                    if (event.key.keysym.unicode < 127) {
                        if ((event.key.keysym.mod & KMOD_ALT) != 0) {
                            return 0x100 | event.key.keysym.unicode;
                        } else {
                            return event.key.keysym.unicode;
                        }
                    }
                    break;
            }
        } else if (event.type == SDL_QUIT) {
            return 0xFF;
        } else if (event.type == SDL_KEYUP) { /* silently drop all key up events */
            continue;
        } else {
            break;
        }
    }

    return 0x00; /* unknown key */
}

static void flushKeyUpEvents(void)
{
    int res;
    SDL_Event event;

    for (;;) { /* silently flush all possible 'KEY UP' events */
        SDL_PumpEvents();
        res = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_KEYUP));
        if (res < 1)
            return;
    }
}

int ui_kbhit(void)
{
    int res;

    flushKeyUpEvents();  /* silently flush all possible 'KEY UP' events */
    res = SDL_PollEvent(NULL);

    return (res < 0) ? 0 : res;
}

void ui_cursor_show(void)
{
    cursorstate = 1;
}

void ui_cursor_hide(void)
{
    cursorstate = 0;
}
