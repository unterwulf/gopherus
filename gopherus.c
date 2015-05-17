/**************************************************************************
 * Gopherus - a console-mode gopher client                                *
 * Copyright (C) Mateusz Viste 2013                                       *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                        *
 **************************************************************************/


#include <string.h>  /* strlen() */
#include <stdlib.h>  /* malloc(), getenv() */
#include <stdio.h>   /* sprintf(), fwrite()... */
#include <time.h>    /* time_t */
#include <unistd.h>  /* usleep() */
#include "common.h"
#include "dnscache.h"
#include "embdpage.h"
#include "gopher.h"
#include "history.h"
#include "menuview.h"
#include "net.h"
#include "parseurl.h"
#include "textview.h"
#include "ui.h"
#include "version.h"
#include "wordwrap.h"

static int hex2int(char c)
{
    switch (c) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'a':
        case 'A':
            return 10;
        case 'b':
        case 'B':
            return 11;
        case 'c':
        case 'C':
            return 12;
        case 'd':
        case 'D':
            return 13;
        case 'e':
        case 'E':
            return 14;
        case 'f':
        case 'F':
            return 15;
        default:
            return -1;
    }
}

static void loadcfg(struct gopherusconfig *cfg)
{
    char *defaultcolorscheme = "177047707818141220";
    char *colorstring;
    int x;
    colorstring = getenv("GOPHERUSCOLOR");
    if (colorstring != NULL) {
        if (strlen(colorstring) == 18) {
            for (x = 0; x < 18; x++) {
                if (hex2int(colorstring[x]) < 0) {
                    colorstring = NULL;
                    break;
                }
            }
        } else {
            colorstring = NULL;
        }
    }
    if (colorstring == NULL) colorstring = defaultcolorscheme;
    /* interpret values from the color scheme variable */
    cfg->attr_textnorm = (hex2int(colorstring[0]) << 4) | hex2int(colorstring[1]);
    cfg->attr_statusbarinfo = (hex2int(colorstring[2]) << 4) | hex2int(colorstring[3]);
    cfg->attr_statusbarwarn = (hex2int(colorstring[4]) << 4) | hex2int(colorstring[5]);
    cfg->attr_urlbar = (hex2int(colorstring[6]) << 4) | hex2int(colorstring[7]);
    cfg->attr_urlbardeco = (hex2int(colorstring[8]) << 4) | hex2int(colorstring[9]);
    cfg->attr_menutype = (hex2int(colorstring[10]) << 4) | hex2int(colorstring[11]);
    cfg->attr_menuerr = (hex2int(colorstring[12]) << 4) | hex2int(colorstring[13]);
    cfg->attr_menuselectable = (hex2int(colorstring[14]) << 4) | hex2int(colorstring[15]);
    cfg->attr_menucurrent = (hex2int(colorstring[16]) << 4) | hex2int(colorstring[17]);
}

int is_int_pending(void)
{
    int res = 0;
    while (ui_kbhit()) {
        int key = ui_getkey();
        switch (key) {
            case KEY_ESCAPE:
            case KEY_BACKSPACE:
                res = 1;
                break;
        }
    }
    return res;
}

/* downloads a gopher or http resource and write it to a file or a memory buffer. if *filename is not NULL, the resource will
   be written in the file (but a valid *buffer is still required) */
static long loadfile_buff(const struct url *url, char *buffer, long buffer_max, char *statusbar, char *filename, struct gopherusconfig *cfg)
{
    unsigned long int ipaddr;
    long reslength, byteread, fdlen = 0;
    char statusmsg[128];
    FILE *fd = NULL;
    int headersdone = 0; /* used notably for HTTP, to localize the end of headers */
    time_t lastactivity, curtime;
    if (url->host[0] == '#') { /* embedded start page */
        reslength = load_embedded_page(buffer, url->host + 1);
        /* open file, if downloading to a file */
        if (filename != NULL) {
            fd = fopen(filename, "rb"); /* try to open for read - this should fail */
            if (fd != NULL) {
                set_statusbar(statusbar, "!File already exists! Operation aborted.");
                fclose(fd);
                return -1;
            }
            fd = fopen(filename, "wb"); /* now open for write - this will create the file */
            if (fd == NULL) { /* this should not fail */
                set_statusbar(statusbar, "!Error: could not create the file on disk!");
                fclose(fd);
                return -1;
            }
            fwrite(buffer, 1, reslength, fd);
            fclose(fd);
        }
        return reslength;
    }
    ipaddr = dnscache_ask(url->host);
    if (ipaddr == 0) {
        sprintf(statusmsg, "Resolving '%s'...", url->host);
        draw_statusbar(statusmsg, cfg);
        ipaddr = net_dnsresolve(url->host);
        if (ipaddr == 0) {
            set_statusbar(statusbar, "!DNS resolution failed!");
            return -1;
        }
        dnscache_add(url->host, ipaddr);
    }
    sprintf(statusmsg, "Connecting to %d.%d.%d.%d...", (int)(ipaddr >> 24) & 0xFF, (int)(ipaddr >> 16) & 0xFF, (int)(ipaddr >> 8) & 0xFF, (int)(ipaddr & 0xFF));
    draw_statusbar(statusmsg, cfg);

    if (net_connect(ipaddr, url->port) != 0) {
        set_statusbar(statusbar, "!Connection error!");
        return -1;
    }
    if (url->protocol == PARSEURL_PROTO_HTTP) { /* http */
        sprintf(buffer, "GET /%s HTTP/1.0\r\nHOST: %s\r\nUSER-AGENT: Gopherus v" VERSION "\r\n\r\n", url->selector, url->host);
    } else { /* gopher */
        sprintf(buffer, "%s\r\n", url->selector);
    }
    if (net_send(buffer, strlen(buffer)) != (int)strlen(buffer)) {
        set_statusbar(statusbar, "!send() error!");
        net_close();
        return -1;
    }
    /* prepare timers */
    lastactivity = time(NULL);
    curtime = lastactivity;
    /* open file, if downloading to a file */
    if (filename != NULL) {
        fd = fopen(filename, "rb"); /* try to open for read - this should fail */
        if (fd != NULL) {
            set_statusbar(statusbar, "!File already exists! Operation aborted.");
            fclose(fd);
            net_abort();
            return -1;
        }
        fd = fopen(filename, "wb"); /* now open for write - this will create the file */
        if (fd == NULL) { /* this should not fail */
            set_statusbar(statusbar, "!Error: could not create the file on disk!");
            fclose(fd);
            net_abort();
            return -1;
        }
    }
    /* receive answer */
    reslength = 0;
    for (;;) {
        if (buffer_max + fdlen - reslength < 1) { /* too much data! */
            set_statusbar(statusbar, "!Error: Server's answer is too long!");
            reslength = -1;
            break;
        }
        byteread = net_recv(buffer + (reslength - fdlen), buffer_max + fdlen - reslength);
        curtime = time(NULL);
        if (byteread < 0) break; /* end of connection */

        if (is_int_pending()) {
            set_statusbar(statusbar, "Connection aborted by the user.");
            reslength = -1;
            break;
        }

        if (byteread > 0) {
            lastactivity = curtime;
            reslength += byteread;
            /* if protocol is http, ignore headers */
            if ((url->protocol == PARSEURL_PROTO_HTTP) && (headersdone == 0)) {
                int i;
                for (i = 0; i < reslength - 2; i++) {
                    if (buffer[i] == '\n') {
                        if (buffer[i + 1] == '\r') i++; /* skip CR if following */
                        if (buffer[i + 1] == '\n') {
                            i += 2;
                            headersdone = reslength;
                            for (reslength = 0; i < headersdone; i++) buffer[reslength++] = buffer[i];
                            break;
                        }
                    }
                }
            } else {
                sprintf(statusmsg, "Downloading... [%ld bytes]", reslength);
                set_statusbar(statusbar, statusmsg);
                draw_statusbar(statusbar, cfg);
                if ((fd != NULL) && (reslength - fdlen > 4096)) { /* if downloading to file, write stuff to disk */
                    int writeres = fwrite(buffer, 1, reslength - fdlen, fd);
                    if (writeres < 0) writeres = 0;
                    fdlen += writeres;
                }
            }
        } else {
            if (curtime - lastactivity > 2) {
                if (curtime - lastactivity > 20) { /* TIMEOUT! */
                    set_statusbar(statusbar, "!Timeout while waiting for data!");
                    reslength = -1;
                    break;
                } else {
                    usleep(250000);  /* give the cpu some time up (250ms), the transfer is really slow */
                }
            }
        }
    }

    if (reslength >= 0) {
        statusmsg[0] = 0;
        draw_statusbar(statusmsg, cfg);
        net_close();
    } else {
        net_abort();
    }

    if (fd != NULL) { /* finish the buffer */
        char tmpmsg[80];
        if (reslength - fdlen > 0) { /* if anything left in the buffer, write it now */
            fdlen += fwrite(buffer, 1, reslength - fdlen, fd);
        }
        fclose(fd);
        sprintf(tmpmsg, "Saved %ld bytes on disk", fdlen);
        set_statusbar(statusbar, tmpmsg);
    }

    return reslength;
}

#define buffersize 1024*1024

static void mainloop(struct gopherus *g)
{
    int exitflag;
    int bufferlen;

    for (;;) {
        struct url *url = &(g->history->url); /* a shortcut */

        if ((url->itemtype == GOPHER_ITEM_FILE) ||
            (url->itemtype == GOPHER_ITEM_DIR) ||
            (url->itemtype == GOPHER_ITEM_INDEX_SEARCH_SERVER) ||
            (url->itemtype == GOPHER_ITEM_HTML)) { /* if it's a displayable item type... */
            draw_urlbar(url, &g->cfg);

            if (g->history->cache == NULL) { /* reload the resource if not in cache already */
                bufferlen = loadfile_buff(url, g->buf, buffersize, g->statusbar, NULL, &g->cfg);
                if (bufferlen < 0) {
                    history_back(&g->history);
                    continue;
                } else {
                    history_cleanupcache(g->history);
                    g->history->cache = malloc(bufferlen);
                    if (g->history->cache == NULL) {
                        sprintf(g->statusbar, "Out of memory!");
                        exitflag = 1;
                        break;
                    }
                    if (bufferlen > 0) memcpy(g->history->cache, g->buf, bufferlen);
                    g->history->cachesize = bufferlen;
                }
            }

            switch (url->itemtype) {
                case GOPHER_ITEM_FILE: /* text file */
                    exitflag = display_text(g, TXT_FORMAT_RAW);
                    break;
                case GOPHER_ITEM_HTML: /* html file */
                    exitflag = display_text(g, TXT_FORMAT_HTM);
                    break;
                case GOPHER_ITEM_DIR: /* menu */
                case GOPHER_ITEM_INDEX_SEARCH_SERVER: /* query result (also a menu) */
                    exitflag = display_menu(g);
                    break;
                default:
                    set_statusbar(g->statusbar, "Fatal error: got an unhandled itemtype!");
                    exitflag = DISPLAY_ORDER_QUIT;
                    break;
            }

            if (exitflag == DISPLAY_ORDER_BACK) {
                history_back(&(g->history));
            } else if (exitflag == DISPLAY_ORDER_REFR) {
                free(g->history->cache);
                g->history->cache = NULL;
                g->history->cachesize = 0;
                g->history->displaymemory[0] = -1;
                g->history->displaymemory[1] = -1;
            } else if (exitflag == DISPLAY_ORDER_QUIT) {
                break;
            }
        } else { /* the itemtype is not one of the internally displayable types -> ask to download it */
            char filename[64] = {0};
            static const char prompt[] = "Download as: ";
            char *lastslash = strrchr(url->selector, '/');
            set_statusbar(filename, ""); /* make sure to clear out the status bar */
            draw_statusbar(filename, &g->cfg);
            ui_cputs(prompt, 0x70, 0, ui_rows - 1);
            if (lastslash)
                strncpy(filename, lastslash + 1, sizeof filename - 1);
            if (editstring(filename, 63, ui_cols - (sizeof prompt - 1), sizeof prompt - 1, ui_rows - 1, 0x70, NULL) != 0) {
                loadfile_buff(url, g->buf, buffersize, g->statusbar, filename, &g->cfg);
            }
            history_back(&(g->history));
        }
    }
}

int main(int argc, char **argv)
{
    struct gopherus g;
    struct url start_url;
    char start_url_str[] = "gopher://#welcome";

    memset(&g, '\0', sizeof g);

    /* Load configuration (or defaults) */
    loadcfg(&g.cfg);

    ui_init();

    parse_url(start_url_str, &start_url);

    if (history_add(&g.history, &start_url) != 0) {
        ui_puts("Out of memory.");
        return 2;
    }

    if (argc > 1) { /* if some params have been received, parse them */
        int i;
        int goturl = 0;

        for (i = 1; i < argc; i++) {
            struct url next_url;

            if ((argv[i][0] == '/') || (argv[i][0] == '-')) { /* unknown parameter */
                ui_puts("Gopherus v" VERSION " Copyright (C) Mateusz Viste " DATE);
                ui_puts("");
                ui_puts("Usage: gopherus [url]");
                ui_puts("");
                return 1;
            }
            if (goturl != 0) {
                ui_puts("Invalid parameters list.");
                return 1;
            }
            if (parse_url(argv[1], &next_url) != 0) {
                ui_puts("Invalid URL!");
                return 1;
            }
            goturl = 1;
            history_add(&g.history, &next_url);
            if (g.history == NULL) {
                ui_puts("Out of memory.");
                return 2;
            }
        }
    }

    g.buf = malloc(buffersize);

    if (g.buf == NULL) {
        char message[128];
        sprintf(message, "Out of memory. Could not allocate buffer of %d bytes.", buffersize);
        ui_puts(message);
        return 2;
    }

    if (net_init() != 0) {
        ui_puts("Network subsystem initialization failed!");
        free(g.buf);
        return 3;
    }

    ui_cursor_hide();
    ui_cls();

    mainloop(&g);

    ui_cls();
    ui_cursor_show();

    if (g.statusbar[0] != 0)
        ui_puts(g.statusbar); /* we might have here an error message to show */

    /* Free the main buffer */
    free(g.buf);
    /* unallocate all the history */
    history_flush(g.history);

    return 0;
}
