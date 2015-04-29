/*
 * This file is part of the Gopherus project
 * Copyright (C) Mateusz Viste 2013
 * 
 */

#include <stdio.h>
#include "wordwrap.h"

int main(int argc, char **argv) {
  char *strptr;
  char res[128];
  if (argc != 2) {
    puts("Usage: wraptest teststring");
    return(0);
  }
  for (strptr = wordwrap(argv[1], res, 16); ; strptr = wordwrap(strptr, res, 16)) {
    printf("|%s|\r\n", res);
    if (strptr == NULL) break;
  }
  return(0);
}
