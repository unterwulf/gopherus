/*
 * This file is part of the Gopherus project
 * Copyright (C) Mateusz Viste 2013
 * 
 * Provides all network functions used by Gopherus, wrapped around POSIX (BSD) sockets.
 */

#include <stdlib.h>  /* NULL */
#include <winsock2.h> /* socket() */
#include <stdio.h> /* sprintf() */
#include <unistd.h> /* close() */
#include <errno.h> /* EAGAIN, EWOULDBLOCK... */
#include <stdint.h> /* uint32_t */

#include "net.h" /* include self for control */


struct netwrap {
  int fd;
};


/* this is a wrapper around the wattcp lookup_host(). returns 0 if resolution fails. */
unsigned long net_dnsresolve(const char *name) {
  struct hostent *hent;
  unsigned long res;
  if ((hent = gethostbyname(name)) == NULL) {
    return(0);
  }
  res = htonl(*((uint32_t *)(hent->h_addr)));
  return(res);
}


/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init() {
  int iResult;
  WSADATA wsaData;
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  return(iResult);
}


struct net_tcpsocket *net_connect(unsigned long ipaddr, int port) {
  struct netwrap *s;
  struct sockaddr_in remote;
  struct net_tcpsocket *result;
  char ipstr[64];
  sprintf(ipstr, "%lu.%lu.%lu.%lu", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);
  s = malloc(sizeof(struct netwrap));
  if (s == NULL) return(NULL);
  s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s->fd < 0) {
    free(s);
    return(NULL);
  }
  result = malloc(sizeof(struct net_tcpsocket));
  if (result == NULL) {
    close(s->fd);
    free(s);
    return(NULL);
  }
  remote.sin_family = AF_INET;  /* Proto family (IPv4) */
  remote.sin_addr.s_addr = inet_addr(ipstr); /* set dst IP address */
  remote.sin_port = htons(port); /* set the dst port */
  if (connect(s->fd, (struct sockaddr *)&remote, sizeof(struct sockaddr)) < 0) {
    close(s->fd);
    free(s);
    free(result);
    return(NULL);
  }
  result->sock = s;
  return(result);
}


/* Sends data on socket 'socket'.
   Returns the number of bytes sent on success, and <0 otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_send(struct net_tcpsocket *socket, char *line, int len) {
  int res;
  res = send(((struct netwrap *)(socket->sock))->fd, line, len, 0);
  return(res);
}


/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_recv(struct net_tcpsocket *socket, char *buff, int maxlen) {
  int res;
  int realsocket;
  fd_set rfds;
  struct timeval tv;
  realsocket = ((struct netwrap *)socket->sock)->fd;
  /* Use select() to wait up to 100ms if nothing awaits on the socket (spares some CPU time) */
  FD_ZERO(&rfds);
  FD_SET(realsocket, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  res = select(realsocket + 1, &rfds, NULL, NULL, &tv);
  if (res < 0) return(-1);
  if (res == 0) return(0);
  /* read the stuff now (if any) */
  res = recv(realsocket, buff, maxlen, 0);
  if (res == 0) return(-1); /* the peer performed an orderly shutdown */
  return(res);
}


/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket) {
  close(((struct netwrap *)(socket->sock))->fd);
  free(socket->sock);
}


/* Close the 'sock' socket immediately (to be used when the peer is behaving wrongly) - this is much faster than net_close(). */
void net_abort(struct net_tcpsocket *socket) {
  net_close(socket);
}
