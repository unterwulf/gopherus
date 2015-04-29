/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#include <stdlib.h>    /* NULL */
#include "wordwrap.h"  /* include self for control */

/* fills *line with part or totality of original *str and return a pointer of *str where to start next iteration */
char *wordwrap(char *str, char *line, int width) {
  int x, lastspace = 0;
  for (x = 0; ; x++) {
    line[x] = str[x];
    if (str[x] == 0) return(NULL);
    if (line[x] == '\t') line[x] = ' '; /* replace all TABs with spaces */
    if (line[x] == ' ') lastspace = x;
    if (x == width) break;
    if (str[x] == '\n') {
      if (x > 0) { /* if it's part of a CR/LF couple, delete them both */
        if (str[x - 1] == '\r') line[x - 1] = 0;
      }
      line[x] = 0;
      return(str + x + 1);
    }
  }
  if (lastspace == 0) { /* I have to cut it in a dumb way */
      line[x] = 0;
      return(str + x);
    } else { /* cut it in word boundary */
      line[lastspace] = 0;
      str += lastspace;
      while (*str == ' ') str += 1;
      if (*str == 0) return(NULL);
      return(str);
  }
}
