/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef NET_H
#define NET_H

extern int is_int_pending(void);

/* this is a wrapper around the wattcp lookup_host(), but with a small integrated cache */
unsigned long net_dnsresolve(const char *name);

/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init(void);

/* connects to a IPv4 host and returns 0 on success, or non-zero otherwise */
int net_connect(unsigned long ipaddr, unsigned short port);

/* Sends data on the socket.
Returns the number of bytes sent on success, and <0 otherwise. */
int net_send(const char *buf, int len);

/* Reads data from the socket and write it into buffer 'buf', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. */
int net_recv(char *buf, int maxlen);

/* Close the socket. */
void net_close(void);

/* Close the socket immediately (to be used when the peer is behaving wrongly) - this is much faster than net_close(). */
void net_abort(void);

#endif
