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
unsigned long dnscache_ask(char *host) {
  int x;
  time_t curtime = time(NULL);
  for (x = 0; x < MAXENTRIES; x++) {
    if (curtime - dnscache_table4[x].inserttime < CACHETIME) {
      if (strcasecmp(host, dnscache_table4[x].host) == 0) return(dnscache_table4[x].addr);
    }
  }
  return(0);
}


/* adds a new entry to the DNS cache */
void dnscache_add(char *host, unsigned long ipaddr) {
  int x, oldest = 0;
  /* time_t curtime = time(NULL); */
  if (strlen(host) > MAXHOSTLEN) return; /* if host len too long, abort */
  for (x = 0; x < MAXENTRIES; x++) {
    if (dnscache_table4[x].inserttime < dnscache_table4[oldest].inserttime) oldest = x; /* remember the oldest entry */
    if (dnscache_table4[x].inserttime > 0) { /* check if it's an already known host */
      if (strcasecmp(dnscache_table4[x].host, host) == 0) {
        oldest = x;
        break;
      }
    }
  }
  /* remove the oldest entry */
  /* dnscache_table4[oldest].inserttime = curtime;
  strcpy(dnscache_table4[oldest].host, host); */
  dnscache_table4[oldest].addr = ipaddr;
}
