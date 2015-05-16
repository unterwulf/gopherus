/*
 * This file is part of the gopherus project.
 */

#include <stdio.h>
#include <string.h>   /* strstr() */
#include <stdlib.h>   /* atoi() */
#include "gopher.h"
#include "parseurl.h"
#include "snprintf.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int parse_url(char *url_str, struct url *url)
{
    int parserstate = 0;
    char *port_str;
    size_t i;

    memset(url, '\0', sizeof *url);
    /* set default values */
    url->protocol = PARSEURL_PROTO_GOPHER;
    url->port = 70;
    url->itemtype = GOPHER_ITEM_DIR;
    url->selector = url_str + strlen(url_str);

    /* skip the protocol part, if present */
    for (i = 0; url_str[i] != 0; i++) {
        if (url_str[i] == '/') { /* no protocol */
            url->protocol = PARSEURL_PROTO_GOPHER;
            break;
        }
        if (url_str[i] == ':') { /* found a colon. check if it's for proto declaration */
            if (url_str[i + 1] == '/') {
                if (url_str[i + 2] == '/') {
                    char *protostr = url_str;
                    url_str[i] = 0;
                    url_str += i + 3;
                    if (strcasecmp(protostr, "gopher") == 0) {
                        url->protocol = PARSEURL_PROTO_GOPHER;
                    } else if (strcasecmp(protostr, "http") == 0) {
                        url->protocol = PARSEURL_PROTO_HTTP;
                        url->port = 80; /* default port is 80 for HTTP */
                        url->itemtype = GOPHER_ITEM_HTML;
                    } else {
                        url->protocol = PARSEURL_PROTO_UNKNOWN;
                    }
                    break;
                }
            }
            url->protocol = PARSEURL_PROTO_GOPHER;
            break;
        }
    }

    /* start reading the url_str */
    url->host = url_str;

    for (; parserstate < 4; url_str++) {
        switch (parserstate) {
            case 0:  /* reading host */
                if (*url_str == ':') { /* a port will follow */
                    *url_str = '\0';
                    port_str = url_str + 1;
                    parserstate = 1;
                } else if (*url_str == '/') { /* gopher type will follow */
                    *url_str = 0;
                    parserstate = 2;
                } else if (*url_str == '\0') { /* end of url_str */
                    parserstate = 4;
                }
                break;
            case 1:  /* reading port */
                if (*url_str == '\0') { /* end of url_str */
                    url->port = atoi(port_str);
                    parserstate = 4;
                } else if (*url_str == '/') {
                    *url_str = 0;
                    url->port = atoi(port_str);
                    parserstate = 2; /* gopher type follows */
                }
                break;
            case 2:  /* reading itemtype */
                if (url->protocol == PARSEURL_PROTO_GOPHER) { /* if non-Gopher, skip the itemtype */
                    if (*url_str != '\0') {
                        url->itemtype = *url_str;
                        parserstate = 3;
                    } else {
                        parserstate = 4;
                    }
                } else {
                    parserstate = 3; /* go right to the url_str part now */
                    url_str--;
                }
                break;
            case 3:
                url->selector = url_str;
                parserstate = 4;
                break;
        }
    }

    return (url->protocol == PARSEURL_PROTO_UNKNOWN);
}

static size_t build_http_url(char *str, size_t size, const struct url *url)
{
    static const char *protoname = "http://";
    size_t len;

    if (size == 0)
        return 0;

    len = (url->port == 80)
        ? snprintf(str, size, "%s%s/%s", protoname, url->host, url->selector)
        : snprintf(str, size, "%s%s:%u/%s", protoname, url->host, url->port, url->selector);

    return MIN(len, size - 1);
}

static size_t build_gopher_url(char *str, size_t size, const struct url *url)
{
    static const char *protoname = "gopher://";
    const char *selector = url->selector;
    size_t len = 0;

    if (size == 0)
        return 0;

    /* detect special hURL links */
    if (url->itemtype == GOPHER_ITEM_HTML) {
        if ((strstr(selector, "URL:") == selector) ||
            (strstr(selector, "/URL:") == selector)) {
            selector += 4 + (selector[0] == '/');
            len = snprintf(str, size, "%s", selector);
        }
    } else if (!url->host[0]) {
        /* if empty host, return only the gopher:// string */
        len = snprintf(str, size, "%s", protoname);
    } else {
        len = (url->port == 70)
            ? snprintf(str, size, "%s%s/%c", protoname, url->host, url->itemtype)
            : snprintf(str, size, "%s%s:%u/%c", protoname, url->host, url->port, url->itemtype);

        for (; *selector && (len < size - 1); selector++) {
            if (((unsigned)*selector <= 0x1F) || ((unsigned)*selector >= 0x80)) { /* encode unsafe chars - RFC 1738: */
                if (len + 2 < size) {                         /* URLs are written only with the graphic printable characters of the  */
                    str[len++] = '%';                           /* US-ASCII coded character set. The octets 80-FF hexadecimal are not  */
                    str[len++] = '0' + (*selector >> 4);        /* used in US-ASCII, and the octets 00-1F and 7F hexadecimal represent */
                    str[len++] = '0' + (*selector & 0x0F);      /* control characters; these must be encoded. */
                }
            } else {
                str[len++] = *selector;
            }
        }

        if (len < size)
            str[len] = '\0';
    }

    return MIN(len, size - 1);
}

/* computes an URL string from exploded parts and returns its length */
size_t build_url(char *str, size_t size, const struct url *url)
{
    return (url->protocol == PARSEURL_PROTO_HTTP)
        ? build_http_url(str, size, url)
        : build_gopher_url(str, size, url);
}
