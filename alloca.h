/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2015 Vitaly Sinilin
 */

#ifndef ALLOCA_H
#define ALLOCA_H

#if defined __BORLANDC__ || defined __MINGW32__
#include <malloc.h>
#elif defined __DJGPP__
#include <stdlib.h>
#else
#include <alloca.h>
#endif

#endif
