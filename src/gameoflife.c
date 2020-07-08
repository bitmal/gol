#include "gameoflife.h"
#include "constants.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

struct rect
{
    i32 bottom;
    i32 right;
    i32 top;
    i32 left;
};

struct entity
{
    b32 isAlive;
    i32 x;
    i32 y;
};

static i32 _g_cam_x;
static i32 _g_cam_y;
static struct entity *_g_entities;
static i32 *_g_entityChangeBuffer;
static i32 _g_entityChangeCapacity;

static void
_gol_clear_buffer(u8 *buffer, i32 bufferWidth, i32 bufferHeight, i32 bpp, u8 *color)
{
    u32 *pixels = (u32 *)buffer;

    for (i32 y = 0; y < bufferHeight; ++y)
    {
        for (i32 x = 0; x < bufferWidth; ++x)
        {
            pixels[y*bufferWidth + x] = *(u32 *)color;
        }
    }
}

static void
_gol_set_pixel(u8 *buffer, i32 bufferWidth, i32 bufferHeight, i32 bpp, i32 x, i32 y, u8 *color)
{
    u32 *pixels = (u32 *)buffer;
    pixels[y*bufferWidth + x] = *(u32 *)color;
}

static struct rect
_gol_clip_rect(i32 bufferWidth, i32 bufferHeight, i32 x, i32 y, i32 width, i32 height)
{
    struct rect bounds;

    if (x < 0)
    {
        bounds.left = 0;
    }
    else if (x >= bufferWidth)
    {
        bounds.left = bufferWidth - 1;
    }
    else
    {
        bounds.left = x;
    }

    i32 right = x + width;
    if (right < 0)
    {
        bounds.right = 0;
    }
    else if (right >= bufferWidth)
    {
        bounds.right = bufferWidth - 1;
    }
    else
    {
        bounds.right = right;
    }
    
    if (y < 0)
    {
        bounds.bottom = 0;
    }
    else if (y >= bufferHeight)
    {
        bounds.bottom = bufferHeight - 1;
    }
    else
    {
        bounds.bottom = y;
    }

    i32 top = y + height;
    if (top < 0)
    {
        bounds.top = 0;
    }
    else if (top >= bufferHeight)
    {
        bounds.top = bufferHeight - 1;
    }
    else
    {
        bounds.top = top;
    }

    return bounds;
}

static void
_gol_grid_to_pixel_coords(i32 bufferWidth, i32 bufferHeight, i32 bpp, i32 mutCoords[2])
{
    mutCoords[0] = (i32)roundf((real32)(mutCoords[0] - _g_cam_x)*(real32)bufferWidth/(real32)GRID_WIDTH);
    mutCoords[1] = (i32)roundf((real32)(mutCoords[1] - _g_cam_y)*(real32)bufferHeight/(real32)GRID_HEIGHT);
}

static void
_gol_draw_rect(u8 *buffer, i32 bufferWidth, i32 bufferHeight, i32 bpp, i32 x, i32 y, i32 width, i32 height, u8 *color)
{
    u32 *pixels = (u32 *)buffer;

    struct rect bounds = _gol_clip_rect(bufferWidth, bufferHeight, x, y, width, height);

    for (i32 y1 = bounds.bottom; y1 < (bounds.bottom + (bounds.top - bounds.bottom)); ++y1)
    {
        for (i32 x1 = bounds.left; x1 < (bounds.left + (bounds.right - bounds.left)); ++x1)
        {
            pixels[y1*bufferWidth + x1] = *(u32 *)color;
        }
    }
}

static void
gol_init(const char *entityAllocGrid, i32 entityAllocWidth, i32 entityAllocHeight)
{
    _g_entities = malloc(sizeof(struct entity)*GRID_WIDTH*GRID_HEIGHT);

    for (i32 y = 0; y < GRID_HEIGHT; ++y)
    {
        for (i32 x = 0; x < GRID_WIDTH; ++x)
        {
            i32 index = y*GRID_WIDTH + x;

            _g_entities[index].isAlive = B32_FALSE;
            _g_entities[index].x = x;
            _g_entities[index].y = y;
        }
    }

    for (i32 y = 0; y < entityAllocHeight; ++y)
    {
        for (i32 x = 0; x < entityAllocWidth; ++x)
        {
            i32 entityIndex = ((GRID_HEIGHT/2 - entityAllocHeight/2 - 1) + y)*GRID_WIDTH + (GRID_WIDTH/2 - entityAllocWidth/2 - 1) + x;
            i32 gridIndex = (entityAllocHeight - y - 1)*entityAllocWidth + x;

            _g_entities[entityIndex].isAlive = entityAllocGrid[gridIndex] == ENTITY_ALIVE_SYMBOL ? B32_TRUE : B32_FALSE;
            _g_entities[entityIndex].x = (GRID_WIDTH/2 - entityAllocWidth/2 - 1) + x;
            _g_entities[entityIndex].y = (GRID_HEIGHT/2 - entityAllocHeight/2 - 1) + y;
        }
    }
}

static void
gol_update()
{
    i32 entityChangeLength = 0;

    for (i32 y = 0; y < GRID_HEIGHT; ++y)
    {
        for (i32 x = 0; x < GRID_WIDTH; ++x)
        {
            i32 entityIndex = y*GRID_WIDTH + x;

            i32 aliveNeighbourCount = 0;
            for (i32 i = -1; i <= 1; ++i)
            {
                i32 yi = y + i;

                if (yi < 0)
                {
                    yi = GRID_HEIGHT - 1;
                }
                else if (yi >= GRID_HEIGHT)
                {
                    yi = 0;
                }

                for (i32 j = -1; j <= 1; ++j)
                {
                    if (i == 0 && j == 0)
                        continue;

                    i32 xj = x + j;

                    if (xj < 0)
                    {
                        xj = GRID_WIDTH - 1;
                    }
                    else if (xj >= GRID_WIDTH)
                    {
                        xj = 0;
                    }

                    if (_g_entities[yi*GRID_WIDTH + xj].isAlive)
                    {
                        ++aliveNeighbourCount;
                    }
                }
            }

            if (((aliveNeighbourCount == 3) && (!_g_entities[entityIndex].isAlive)) ||
                ((aliveNeighbourCount < 2 || aliveNeighbourCount > 3) && _g_entities[entityIndex].isAlive))
            {
                if (_g_entityChangeCapacity < (entityChangeLength + 1))
                {
                    if (_g_entityChangeBuffer)
                    {
                        _g_entityChangeBuffer = realloc(_g_entityChangeBuffer, 
                            sizeof(i32)*(_g_entityChangeCapacity *= 2));
                    }
                    else
                    {
                        _g_entityChangeBuffer = malloc(sizeof(i32)*(_g_entityChangeCapacity = 10));
                    }

                }
                
                _g_entityChangeBuffer[entityChangeLength++] = entityIndex;
            }
        }
    }

    for (i32 i = 0; i < entityChangeLength; ++i)
    {
        _g_entities[_g_entityChangeBuffer[i]].isAlive = !_g_entities[_g_entityChangeBuffer[i]].isAlive;
    }
}

static void
gol_draw(u8 *buffer, i32 bufferWidth, i32 bufferHeight, i32 bpp)
{
    u32 clearColor = 0x0;
    _gol_clear_buffer(buffer, bufferWidth, bufferHeight, bpp, (u8 *)&clearColor);
    
    for (i32 y = 0; y < GRID_HEIGHT; ++y)
    {
        for (i32 x = 0; x < GRID_WIDTH; ++x)
        {
            i32 entityIndex = GRID_WIDTH*y + x;

            if (_g_entities[entityIndex].isAlive)
            {
                u32 rectColor = (u32)(((real32)rand()/(real32)RAND_MAX)*(real32)0xFFFFFFFF) | 0xFF000000;
                i32 rectCoords[] = {_g_entities[entityIndex].x, _g_entities[entityIndex].y};
                _gol_grid_to_pixel_coords(bufferWidth, bufferHeight, bpp, rectCoords);
                _gol_draw_rect(buffer, bufferWidth, bufferHeight, bpp, rectCoords[0], rectCoords[1], 
                    (i32)roundf((real32)bufferWidth/GRID_WIDTH),
                    (i32)roundf((real32)bufferHeight/GRID_HEIGHT),
                    (u8 *)&rectColor);
            }
        }
    }
}