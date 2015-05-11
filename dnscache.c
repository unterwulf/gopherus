/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#include <time.h>
#include <string.h>
#include "dnscache.h"

#define MAXENTRIES 16
#define MAXHOSTLEN 31
#define CACHETIME 120

struct dnscache_type4 {
    char host[MAXHOSTLEN + 1];
    unsigned long addr;
    time_t inserttime;
};

static struct dnscache_type4 dnscache_table4[MAXENTRIES];

/* returns the ip addr if host found in cache, 0 otherwise */
unsigned long dnscache_ask(const char *host)
{
    size_t i;
    time_t curtime = time(NULL);

    for (i = 0; i < MAXENTRIES; i++)
        if (curtime - dnscache_table4[i].inserttime < CACHETIME)
            if (!strcasecmp(host, dnscache_table4[i].host))
                return dnscache_table4[i].addr;

    return 0;
}

/* adds a new entry to the DNS cache */
void dnscache_add(const char *host, unsigned long ipaddr)
{
    size_t i;
    size_t oldest = 0;

    if (strlen(host) > MAXHOSTLEN)
        return; /* if hostname is too long, just ignore it */

    for (i = 0; i < MAXENTRIES; i++) {
        if (dnscache_table4[i].inserttime < dnscache_table4[oldest].inserttime)
            oldest = i; /* remember the oldest entry */

        if (dnscache_table4[i].inserttime > 0) { /* check if it's an already known host */
            if (!strcasecmp(dnscache_table4[i].host, host)) {
                oldest = i;
                break;
            }
        }
    }

    /* replace the oldest entry */
    strcpy(dnscache_table4[oldest].host, host);
    dnscache_table4[oldest].addr = ipaddr;
    dnscache_table4[oldest].inserttime = time(NULL);
}
