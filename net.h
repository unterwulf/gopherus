/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */


#ifndef NET_H
#define NET_H

struct net_tcpsocket {
  void *sock;
  char *buffer;
  int buffersize;
};

/* this is a wrapper around the wattcp lookup_host(), but with a small integrated cache */
unsigned long net_dnsresolve(const char *name);

/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init();

/* connects to a IPv4 host and returns a socket pointer on success, NULL otherwise */
struct net_tcpsocket *net_connect(unsigned long ipaddr, int port);

/* Sends data on socket 'socket'.
Returns the number of bytes sent on success, and <0 otherwise. */
int net_send(struct net_tcpsocket *socket, char *line, int len);

/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. */
int net_recv(struct net_tcpsocket *socket, char *buff, int maxlen);

/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket);

/* Close the 'sock' socket immediately (to be used when the peer is behaving wrongly) - this is much faster than net_close(). */
void net_abort(struct net_tcpsocket *socket);

#endif
