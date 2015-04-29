 
#include <stdio.h>
#include <stdlib.h>
#include "libtcp.h"


int main() {
  char *host = "www.wp.pl";
  unsigned long hostaddr;
  struct tcpsocket *sock;
  char *result;
  long result_len;
  #define result_size 1024 * 256

  result = malloc(result_size);

  libtcp_init(); /* must be called before using libtcp */

  hostaddr = dnsresolve(host);

  printf("resolved host %s -> %lu.%lu.%lu.%lu\r\n", host, (hostaddr >> 24) & 0xFF, (hostaddr >> 16) & 0xFF, (hostaddr >> 8) & 0xFF, hostaddr & 0xFF);

  sock = libtcp_connect(hostaddr, 80);
  if (sock == NULL) {
      puts("ERROR: Could not connect!");
      return(0);
    } else {
      puts("Conected to remote host.");
  }
  
  libtcp_send(sock, "GET / HTTP/1.0\r\n\r\n", 20);

  result_len = libtcp_recv(sock, result, result_size);
  if (result_len < 0) {
      puts("recv error");
    } else {
      printf("Got %ld bytes:\r\n", result_len);
      puts(result);
  }

  libtcp_close(sock);
  free(result);

  return(0);
}
