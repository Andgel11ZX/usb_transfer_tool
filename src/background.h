
#ifndef _BACKGROUND_H_
#define _BACKGROUND_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <gctypes.h>

extern u8 *picTVBuf;
extern u8 *picDRCBuf;

extern int SIZES[][2];

u8* LoadPicture(char* name, u32 size);



void DrawRectangle(int pos_x, int pos_y, int width, int height, char r,char g, char b, int screen);
void DrawBackground(int screen);


#ifdef __cplusplus
}
#endif

#endif /* _FTP_H_ */
