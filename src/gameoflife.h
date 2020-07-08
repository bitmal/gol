#ifndef __GAMEOFLIFE_H
#define __GAMEOFLIFE_H

#include "types.h"

void
gol_init(const char *entityAllocGrid, i32 entityAllocWidth, i32 entityAllocHeight);

static void
gol_update();

void
gol_draw(u8 *buffer, i32 bufferWidth, i32 bufferHeight, i32 bpp);

#endif