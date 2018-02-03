#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include "background.h"
#include <gctypes.h>
#include "dynamic_libs/os_functions.h"
#include "system/memory.h"

#include "drcBack.h"
#include "tvBack.h"

#define TV_WIDTH 1280
#define DRC_WIDTH 854
#define TV_HEIGHT 720
#define DRC_HEIGHT 480

extern int SIZES[][2] = {{TV_WIDTH, TV_HEIGHT}, {DRC_WIDTH, DRC_HEIGHT}};


u8 *LoadPicture(char *name, u32 size)
{
    u8 *buf = NULL;
    u32 fileSize = 0;
    if (LoadFileToMem(name, &buf, &fileSize) < size)
    {
        free(buf);
        return NULL;
    }

    return buf;
}


void DrawRectangle(int pos_x, int pos_y, int width, int height, char r,char g, char b, int screen)
{

    uint32_t num = (r << 24) | (g << 16) | (b << 8) | 255;

    for(int x=pos_x;x<pos_x+width;x++)
    {
        for(int y=pos_y;y<pos_y+height;y++)
        {
             OSScreenPutPixelEx(screen, x, y, num);
        }
    }
}

void DrawBackground(int screen)
{
    char r = 0;            //rouge
    char g = 0;            //vert
    char b = 0;            //bleu
    const char a = 255;    //alpha ne pas changer la valeur
    unsigned int pos = 18; //saut de l'entete du header TGA

    u8 *buffer = (screen == 0 ? tvBack : drcBack);

    if (buffer == NULL)
        return;
    
    for (int y = 0; y < SIZES[screen][1]; y++)
    {
        for (int x = 0; x < SIZES[screen][0]; x++)
        {
            b = *(buffer + pos);
            g = *(buffer + pos + 1);
            r = *(buffer + pos + 2);
            pos += 3;

            uint32_t num = (r << 24) | (g << 16) | (b << 8) | a;
            OSScreenPutPixelEx(screen, x, SIZES[screen][1] - y, num);
        }
    }
}
