/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef startpg_h_sentinel
  #define startpg_h_sentinel
  /* loads the embedded start page into a memory buffer and returns */
  int loadembeddedstartpage(char *buffer, char *selector, char *pVer, char *pDate);
#endif
