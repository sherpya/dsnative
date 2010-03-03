/*
 * dstool
 * Copyright (c) 2010 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 * dstool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * dstoolis distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dstool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define DSN_OK 0
#define DSN_API_VERSION 4

typedef struct _DSVideoCodec DSVideoCodec;
typedef DSVideoCodec * (WINAPI *funcDSOpenVideoCodec)(const char *dll, const GUID guid, BITMAPINFOHEADER* bih,
                                                        unsigned int outfmt, float fps, const char *filename, int *err);
typedef void (WINAPI *funcDSCloseVideoCodec)(DSVideoCodec *codec);
typedef int (WINAPI *funcDSVideoDecode)(DSVideoCodec *vcodec, const BYTE *src, int size, double pts,
                                          double *newpts, BYTE *pImage, int keyframe);
typedef int (WINAPI *funcDSVideoResync)(DSVideoCodec *codec, double pts);
typedef const char * (WINAPI *funcDSStrError)(int error);
typedef unsigned int (WINAPI *funcDSGetApiVersion)(void);

static struct
{
    char *filename;
    GUID guid;
} codecs[] = { "ffdshow.ax", { 0x04fe9017, 0xf873, 0x410e, { 0x87, 0x1e, 0xab, 0x91, 0x66, 0x1a, 0x4e, 0xf7 } } };

int main(void)
{
    HMODULE hDs = LoadLibrary("dsnative.dll");
    funcDSOpenVideoCodec DSOpenVideoCodec = (funcDSOpenVideoCodec) GetProcAddress(hDs, "DSOpenVideoCodec");
    funcDSCloseVideoCodec DSCloseVideoCodec = (funcDSCloseVideoCodec) GetProcAddress(hDs, "DSCloseVideoCodec");

    BITMAPINFOHEADER bi;
    memset(&bi, 0, sizeof(bi));
    bi.biSize = sizeof(bi);
    bi.biCompression = mmioFOURCC('X', 'V', 'I', 'D');
    bi.biWidth = 320;
    bi.biHeight = 240;

    DSVideoCodec *c = DSOpenVideoCodec(codecs[0].filename, codecs[0].guid, &bi, mmioFOURCC('Y', 'V', '1', '2'), 25.0f, NULL, NULL);
    if (c) DSCloseVideoCodec(c);
    return 0;
}
