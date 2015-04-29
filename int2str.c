/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#include <stdlib.h>  /* NULL, div() */

/* converts an integer from the range 0..9999999 into a string. returns the length of the computed string, or -1 on error. */
int int2str(char *res, int x) {
  int step, reslen = 0;
  div_t divres;
  if (res == NULL) return(-1);
  if ((x < 0) || (x > 9999999)) {
    *res = 0;
    return(-1);
  }
  if (x < 10) {
      step = 1;
    } else if (x < 100) {
      step = 10;
    } else if (x < 1000) {
      step = 100;
    } else if (x < 10000) {
      step = 1000;
    } else if (x < 100000) {
      step = 10000;
    } else if (x < 1000000) {
      step = 100000;
    } else {
      step = 1000000;
  }
  for (; step > 0; step /= 10) {
    divres = div(x, step);  /* all this could be written as: */
                            /* digit = (x / step) ; x %= step; */
    x = divres.rem;         /* but the div() call is presumably faster, as it (probably) uses a native single CPU instruction */
    res[reslen++] = divres.quot + '0';
  }
  res[reslen] = 0;
  return(reslen);
}
