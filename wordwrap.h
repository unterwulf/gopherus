/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef WORDWRAP_H
#define WORDWRAP_H

/* fills *line with part or totality of original *str and return a pointer of *str where to start next iteration */
char *wordwrap(char *str, char *line, int width);

#endif
