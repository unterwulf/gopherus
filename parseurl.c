/*
 * This file is part of the gopherus project.
 */

#include <stdio.h>
#include <string.h>   /* strstr() */
#include <stdlib.h>   /* atoi() */
#include "gopher.h"
#include "parseurl.h"

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

static int build_http_url(char *res, int maxlen, const struct url *url)
{
    static const char *protoname = "http://";
    const char *host = url->host;
    const char *selector = url->selector;
    int i;

    maxlen--;

    for (i = 0; protoname[i] && (i < maxlen); i++)
        res[i] = protoname[i];

    while (*host && (i < maxlen))
        res[i++] = *host++;

    /* port (optional, only if not 80) */
    if (url->port != 80 && (i + 6 < maxlen)) {
        res[i++] = ':';
        i += sprintf(&res[i], "%u", url->port);
    }

    if (i < maxlen)
        res[i++] = '/';

    while (*selector && (i < maxlen))
        res[i++] = *selector++;

    res[i] = '\0';
    return i;
}

static int build_gopher_url(char *res, int maxlen, const struct url *url)
{
    static const char *protoname = "gopher://";
    const char *host = url->host;
    const char *selector = url->selector;
    int i = 0;

    maxlen--;

    if (maxlen < 2) return -1;
    if (url->port < 1) return -1;
    if (!host || !res || !selector) return -1;
    if (url->itemtype < 33) return -1;

    /* detect special hURL links */
    if (url->itemtype == GOPHER_ITEM_HTML) {
        if ((strstr(selector, "URL:") == selector) || (strstr(selector, "/URL:") == selector)) {
            if (selector[0] == '/') selector++;
            selector += 4;
            i = 0;
            while (*selector && (i < maxlen))
                res[i++] = *selector++;
        }
    } else {
        /* this is a classic gopher location */
        for (i = 0; protoname[i] && (i < maxlen); i++)
            res[i] = protoname[i];

        /* if empty host, return only the gopher:// string */
        if (!host[0]) {
            res[i] = '\0';
            return i;
        }

        /* build the url string */
        while (*host && (i < maxlen))
            res[i++] = *host++;

        if (url->port != 70 && (i + 6 < maxlen)) {
            res[i++] = ':';
            i += sprintf(&res[i], "%u", url->port);
        }

        if (i < maxlen)
            res[i++] = '/';

        if (i < maxlen)
            res[i++] = url->itemtype;

        for (; *selector && (i < maxlen); selector++) {
            if (((unsigned)*selector <= 0x1F) || ((unsigned)*selector >= 0x80)) { /* encode unsafe chars - RFC 1738: */
                if (i + 2 < maxlen) {                         /* URLs are written only with the graphic printable characters of the  */
                    res[i++] = '%';                           /* US-ASCII coded character set. The octets 80-FF hexadecimal are not  */
                    res[i++] = '0' + (*selector >> 4);        /* used in US-ASCII, and the octets 00-1F and 7F hexadecimal represent */
                    res[i++] = '0' + (*selector & 0x0F);      /* control characters; these must be encoded. */
                }
            } else {
                res[i++] = *selector;
            }
        }
    }

    res[i] = '\0';
    return i;
}

/* computes an URL string from exploded gopher parts, and returns its length. Returns -1 on error. */
int build_url(char *res, int maxlen, const struct url *url)
{
    return (url->protocol == PARSEURL_PROTO_HTTP)
        ? build_http_url(res, maxlen, url)
        : build_gopher_url(res, maxlen, url);
}
