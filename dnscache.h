/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */


#ifndef dnscache_h_sentinel
  #define dnscache_h_sentinel
  /* returns the ip addr if host found in cache, 0 otherwise */
  unsigned long dnscache_ask(char *host);

  /* adds a new entry to the DNS cache */
  void dnscache_add(char *host, unsigned long ipaddr);
#endif
