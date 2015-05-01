/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "history.h"
#include "parseurl.h"
#include "textview.h"
#include "ui.h"
#include "wordwrap.h"

int display_text(struct gopherus *g, int txtformat)
{
    char *txtptr;
    char linebuff[80];
    long x, y, firstline, lastline, bufferlen;
    int eof_flag;
    sprintf(linebuff, "file loaded (%ld bytes)", g->history->cachesize);
    set_statusbar(g->statusbar, linebuff);
    /* copy the content of the file into g->buf, and take care to modify dangerous chars and apply formating (if any) */
    bufferlen = 0;

    if (txtformat == TXT_FORMAT_HTM) { /* HTML format */
        int lastcharwasaspace = 0;
        int insidetoken = -1;
        int insidescript = 0;
        char token[8];
        char specialchar[8];
        int insidespecialchar = -1;
        for (x = 0; x < g->history->cachesize; x++) {
            if ((insidescript != 0) && (insidetoken < 0) && (g->history->cache[x] != '<')) continue;
            switch (g->history->cache[x]) {
                case '\t':  /* replace whitespaces by single spaces */
                case '\n':
                case '\r':
                case ' ':
                    if (insidetoken >= 0) {
                        if (insidetoken < 7) token[insidetoken++] = 0;
                        continue;
                    }
                    if (lastcharwasaspace == 0) {
                        g->buf[bufferlen++] = ' ';
                        lastcharwasaspace = 1;
                    }
                    break;
                case '<':
                    lastcharwasaspace = 0;
                    insidetoken = 0;
                    break;
                case '>':
                    lastcharwasaspace = 0;
                    if (insidetoken < 0) continue;
                    token[insidetoken] = 0;
                    insidetoken = -1;
                    if ((strcasecmp(token, "/p") == 0) || (strcasecmp(token, "br") == 0) || (strcasecmp(token, "/tr") == 0) || (strcasecmp(token, "/title") == 0)) {
                        g->buf[bufferlen++] = '\n';
                    } else if (strcasecmp(token, "script") == 0) {
                        insidescript = 1;
                    } else if (strcasecmp(token, "/script") == 0) {
                        insidescript = 0;
                    }
                    break;
                default:
                    lastcharwasaspace = 0;
                    if (insidetoken >= 0) {
                        if (insidetoken < 7) token[insidetoken++] = g->history->cache[x];
                        continue;
                    }
                    if ((insidespecialchar < 0) && (g->history->cache[x] == '&')) {
                        insidespecialchar = 0;
                        continue;
                    }
                    if ((insidespecialchar >= 0) && (insidespecialchar < 7)) {
                        if (g->history->cache[x] != ';') {
                            specialchar[insidespecialchar++] = g->history->cache[x];
                            continue;
                        }
                        specialchar[insidespecialchar] = 0;
                        if (strcasecmp(specialchar, "nbsp") == 0) {
                            g->buf[bufferlen++] = ' ';
                        } else {
                            g->buf[bufferlen++] = '_';
                        }
                        insidespecialchar = -1;
                        continue;
                    }
                    if (g->history->cache[x] < 32) break; /* ignore ascii control chars */
                    g->buf[bufferlen++] = g->history->cache[x]; /* copy everything else */
                    break;
            }
        }
    } else { /* process content as raw text */
        for (x = 0; x < g->history->cachesize; x++) {
            switch (g->history->cache[x]) {
                case 8:     /* replace tabs by 8 spaces */
                    for (y = 0; y < 8; y++) g->buf[bufferlen++] = ' ';
                    break;
                case '\n':  /* preserve line feeds */
                    g->buf[bufferlen++] = '\n';
                    break;
                case '\r':  /* ignore CR chars */
                case 127:   /* as well as DEL chars */
                    break;
                default:
                    if (g->history->cache[x] < 32) break; /* ignore ascii control chars */
                    g->buf[bufferlen++] = g->history->cache[x]; /* copy everything else */
                    break;
            }
        }
        /* check if there is a single . on the last line */
        if ((g->buf[bufferlen - 1] == '\n') && (g->buf[bufferlen - 2] == '.')) bufferlen -= 2;
    }

    /* terminate the g->buf with a NULL terminator */
    g->buf[bufferlen] = 0;
    /* display the file on screen */
    firstline = 0;
    lastline = ui_getrowcount() - 3;

    for (;;) { /* display-control loop */
        y = 0;
        for (txtptr = g->buf; txtptr != NULL; ) {
            txtptr = wordwrap(txtptr, linebuff, 80);
            if (y >= firstline) {
                for (x = 0; linebuff[x] != 0; x++) ui_putchar(linebuff[x], g->cfg.attr_textnorm, x, y + 1 - firstline);
                for (; x < 80; x++) ui_putchar(' ', g->cfg.attr_textnorm, x, y + 1 - firstline);
            }
            y++;
            if (y > lastline) break;
        }
        if (y <= lastline) {
            eof_flag = 1;
            for (; y <= lastline ; y++) { /* fill the rest of the screen (if any left) with blanks */
                for (x = 0; x < 80; x++) ui_putchar(' ', g->cfg.attr_textnorm, x, y + 1 - firstline);
            }
        } else {
            eof_flag = 0;
        }
        draw_statusbar(g->statusbar, &(g->cfg));
        x = ui_getkey();
        switch (x) {
            case KEY_BACKSPACE:
                return DISPLAY_ORDER_BACK;
                break;
            case KEY_TAB:
                if (edit_url(&(g->history), &(g->cfg)) == 0) return DISPLAY_ORDER_NONE;
                break;
            case KEY_ESCAPE:
                if (ask_quit_confirmation(&(g->cfg)) != 0) return DISPLAY_ORDER_QUIT;
                break;
            case KEY_F1: /* help */
                history_add(&(g->history), PARSEURL_PROTO_GOPHER, "#manual", 70, '0', "");
                return DISPLAY_ORDER_NONE;
                break;
            case KEY_F5: /* refresh */
                return DISPLAY_ORDER_REFR;
                break;
            case KEY_F9: /* download */
                history_add(&(g->history), g->history->protocol, g->history->host, g->history->port, '9', g->history->selector);
                return DISPLAY_ORDER_NONE;
                break;
            case KEY_UP:
                if (firstline > 0) {
                    firstline -= 1;
                    lastline -= 1;
                } else {
                    set_statusbar(g->statusbar, "Reached the top of the file");
                }
                break;
            case KEY_DOWN:
                if (eof_flag == 0) {
                    firstline += 1;
                    lastline += 1;
                } else {
                    set_statusbar(g->statusbar, "Reached end of file");
                }
                break;
            case KEY_HOME:
                lastline -= firstline;
                firstline = 0;
                break;
            case KEY_PAGEUP:
                if (firstline > 0) {
                    firstline -= ui_getrowcount() - 3;
                    if (firstline < 0) firstline = 0;
                    lastline = firstline + ui_getrowcount() - 3;
                } else {
                    set_statusbar(g->statusbar, "Reached the top of the file");
                }
                break;
            case KEY_END:
                break;
            case KEY_PAGEDOWN:
                if (eof_flag == 0) {
                    firstline += ui_getrowcount() - 3;
                    lastline += ui_getrowcount() - 3;
                } else {
                    set_statusbar(g->statusbar, "Reached end of file");
                }
                break;
            case KEY_QUIT: /* QUIT IMMEDIATELY */
                return 1;
                break;
            default:  /* unhandled key */
                /* sprintf(linebuff, "Got invalid key: 0x%02lX", x);
                   set_statusbar(g->statusbar, linebuff); */
                break;
        }
    }
}
