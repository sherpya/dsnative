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

typedef struct _DSCodec DSCodec;
typedef DSCodec * (WINAPI *funcDSOpenCodec)(const char *dll, const GUID guid, BITMAPINFOHEADER* bih);
typedef void (WINAPI *funcDSCloseCodec)(DSCodec *codec);
typedef BOOL (WINAPI *funcDSShowPropertyPage)(DSCodec *codec);

//const GUID guid = { 0x09571a4b, 0xf1fe, 0x4c60, { 0x97, 0x60, 0xde, 0x6d, 0x31, 0x0c, 0x7c, 0x31 } };
//const char codec[] = "CoreAVCDecoder.ax";
//const GUID guid = { 0x82CCD3E0, 0xF71A, 0x11D0, { 0x9F, 0xE5, 0x00, 0x60, 0x97, 0x78, 0xEA, 0x66 }};
//const char codec[] = "mpg4ds32.ax";

const GUID guid = { 0x521fb373, 0x7654, 0x49f2, { 0xbd, 0xb1, 0x0c, 0x6e, 0x66, 0x60, 0x71, 0x4f } };
const char codec[] = "wmv8ds32.ax";

//const GUID guid = { 0x4cb63e61, 0xc611, 0x11D0, { 0x83, 0xaa, 0x00, 0x00, 0x92, 0x90, 0x01, 0x84 } };
//const char codec[] = "tm20dec.ax";

//const GUID guid = { 0x5DFA20A4, 0xC343, 0x45DF, { 0xA4, 0x6E, 0xEC, 0x17, 0x11, 0xE4, 0xEE, 0x32 } };
//const char codec[] = "diavc.ax";

int main(void)
{
    HMODULE hDs = LoadLibrary("dsnative.dll");
    funcDSOpenCodec DSOpenCodec = (funcDSOpenCodec) GetProcAddress(hDs, "DSOpenCodec");
    funcDSCloseCodec DSCloseCodec = (funcDSCloseCodec) GetProcAddress(hDs, "DSCloseCodec");
    funcDSShowPropertyPage DSShowPropertyPage = (funcDSShowPropertyPage) GetProcAddress(hDs, "DSShowPropertyPage");
    DSCodec *c = DSOpenCodec(codec, guid, NULL);
    //__asm { int 3 };
    //DSShowPropertyPage(c);
    DSCloseCodec(c);
    return 0;
}
