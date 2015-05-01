/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include "common.h"
#include "history.h"

int display_text(struct historytype **history, struct gopherusconfig *cfg, char *buffer, char *statusbar, int txtformat);

#endif
