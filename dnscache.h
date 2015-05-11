/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef DNSCACHE_H
#define DNSCACHE_H

/* returns the ip addr if host found in cache, 0 otherwise */
unsigned long dnscache_ask(const char *host);

/* adds a new entry to the DNS cache */
void dnscache_add(const char *host, unsigned long ipaddr);

#endif
