/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef wordwrap_h_sentinel
#define wordwrap_h_sentinel
  /* fills *line with part or totality of original *str and return a pointer of *str where to start next iteration */
  char *wordwrap(char *str, char *line, int width);
#endif
