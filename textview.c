/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "gopher.h"
#include "history.h"
#include "parseurl.h"
#include "textview.h"
#include "ui.h"
#include "wordwrap.h"

static int display_text_loop(struct gopherus *g)
{
    long x;
    long firstline = 0;
    long lastline = ui_getrowcount() - 2;
    char *txtptr;
    int eof_flag;
    char linebuff[80];
    int key;

    for (;;) {
        long y = 1;
        long lineno = 0;

        for (txtptr = g->buf; txtptr != NULL && y <= lastline; lineno++) {
            txtptr = wordwrap(txtptr, linebuff, 80);
            if (lineno >= firstline) {
                for (x = 0; linebuff[x] != 0; x++)
                    ui_putchar(linebuff[x], g->cfg.attr_textnorm, x, y);
                for (; x < 80; x++)
                    ui_putchar(' ', g->cfg.attr_textnorm, x, y);
                y++;
            }
        }

        if (y <= lastline) {
            eof_flag = 1;

            /* fill the rest of the screen (if any left) with blanks */
            for (; y <= lastline; y++) {
                for (x = 0; x < 80; x++)
                    ui_putchar(' ', g->cfg.attr_textnorm, x, y);
            }
        } else {
            eof_flag = 0;
        }

        draw_statusbar(g->statusbar, &(g->cfg));
        key = ui_getkey();

        switch (key) {
            case KEY_BACKSPACE:
                return DISPLAY_ORDER_BACK;
            case KEY_TAB:
                if (edit_url(&(g->history), &(g->cfg)) == 0)
                    return DISPLAY_ORDER_NONE;
                break;
            case KEY_ESCAPE:
                if (ask_quit_confirmation(&(g->cfg)) != 0)
                    return DISPLAY_ORDER_QUIT;
                break;
            case KEY_F1: /* help */
                history_add(&(g->history),
                            PARSEURL_PROTO_GOPHER,
                            "#manual", 70, GOPHER_ITEM_FILE, "");
                return DISPLAY_ORDER_NONE;
            case KEY_F5: /* refresh */
                return DISPLAY_ORDER_REFR;
            case KEY_F9: /* download */
                history_add(&(g->history),
                            g->history->protocol,
                            g->history->host,
                            g->history->port,
                            GOPHER_ITEM_BINARY,
                            g->history->selector);
                return DISPLAY_ORDER_NONE;
            case KEY_UP:
                if (firstline > 0) {
                    firstline -= 1;
                } else {
                    set_statusbar(g->statusbar, "Reached the top of the file");
                }
                break;
            case KEY_DOWN:
                if (!eof_flag)
                    firstline++;
                else
                    set_statusbar(g->statusbar, "Reached end of file");
                break;
            case KEY_HOME:
                firstline = 0;
                break;
            case KEY_PAGEUP:
                if (firstline > 0) {
                    firstline -= ui_getrowcount() - 3;
                    if (firstline < 0)
                        firstline = 0;
                } else {
                    set_statusbar(g->statusbar, "Reached the top of the file");
                }
                break;
            case KEY_END:
                break;
            case KEY_PAGEDOWN:
                if (!eof_flag)
                    firstline += ui_getrowcount() - 3;
                else
                    set_statusbar(g->statusbar, "Reached end of file");
                break;
            case KEY_QUIT: /* QUIT IMMEDIATELY */
                return 1;
            default:  /* unhandled key */
                /* sprintf(linebuff, "Got invalid key: 0x%02lX", key);
                   set_statusbar(g->statusbar, linebuff); */
                break;
        }
    }
}

static void process_html(char *dst, const char *src, long slen)
{
    int lastcharwasaspace = 0;
    int insidetoken = -1;
    int insidescript = 0;
    char token[8];
    char specialchar[8];
    int insidespecialchar = -1;
    long dlen = 0;
    long x;

    for (x = 0; x < slen; x++) {
        if ((insidescript != 0) && (insidetoken < 0) && (src[x] != '<'))
            continue;

        switch (src[x]) {
            case '\t':  /* replace whitespaces by single spaces */
            case '\n':
            case '\r':
            case ' ':
                if (insidetoken >= 0) {
                    if (insidetoken < 7) token[insidetoken++] = 0;
                    continue;
                }
                if (lastcharwasaspace == 0) {
                    dst[dlen++] = ' ';
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
                    dst[dlen++] = '\n';
                } else if (strcasecmp(token, "script") == 0) {
                    insidescript = 1;
                } else if (strcasecmp(token, "/script") == 0) {
                    insidescript = 0;
                }
                break;
            default:
                lastcharwasaspace = 0;

                if (insidetoken >= 0) {
                    if (insidetoken < 7) token[insidetoken++] = src[x];
                    continue;
                }

                if ((insidespecialchar < 0) && (src[x] == '&')) {
                    insidespecialchar = 0;
                    continue;
                }

                if ((insidespecialchar >= 0) && (insidespecialchar < 7)) {
                    if (src[x] != ';') {
                        specialchar[insidespecialchar++] = src[x];
                        continue;
                    }
                    specialchar[insidespecialchar] = 0;
                    if (strcasecmp(specialchar, "nbsp") == 0) {
                        dst[dlen++] = ' ';
                    } else {
                        dst[dlen++] = '_';
                    }
                    insidespecialchar = -1;
                    continue;
                }

                if (src[x] < 32)
                    break; /* ignore ascii control chars */
                dst[dlen++] = src[x]; /* copy everything else */
                break;
        }
    }

    /* terminate with a NUL-terminator */
    dst[dlen] = '\0';
}

static void process_plain_text(char *dst, const char *src, long slen)
{
    long dlen = 0;
    int i;

    for (; slen > 0; slen--, src++) {
        switch (*src) {
            case '\t':  /* replace tabs by 8 spaces */
                for (i = 0; i < 8; i++)
                    dst[dlen++] = ' ';
                break;
            case '\n':  /* preserve line feeds */
                dst[dlen++] = '\n';
                break;
            case '\r':  /* ignore CR chars */
            case 127:   /* as well as DEL chars */
                break;
            default:
                if (*src < 32)
                    break; /* ignore ascii control chars */
                dst[dlen++] = *src; /* copy everything else */
                break;
        }
    }

    /* check if there is a single . on the last line */
    if (dlen >= 2 && (dst[dlen-1] == '\n') && (dst[dlen-2] == '.'))
        dlen -= 2;

    /* terminate with a NUL-terminator */
    dst[dlen] = '\0';
}

int display_text(struct gopherus *g, int txtformat)
{
    char buf[80];
    sprintf(buf, "File loaded (%ld bytes)", g->history->cachesize);
    set_statusbar(g->statusbar, buf);

    /* copy the content of the file into g->buf, and take care to modify
     * dangerous chars and apply formating (if any) */

    if (txtformat == TXT_FORMAT_HTM)
        process_html(g->buf, g->history->cache, g->history->cachesize);
    else
        process_plain_text(g->buf, g->history->cache, g->history->cachesize);

    return display_text_loop(g);
}
