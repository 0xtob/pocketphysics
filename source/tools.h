// libNTXM - XM Player Library for the Nintendo DS
// Copyright (C) 2005-2007 Tobias Weyand (0xtob)
//                         me@nitrotracker.tobw.net
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <stdlib.h>
#include <stdio.h>
#include <nds.h>

#ifndef max
#define min(x,y)	((x)<(y)?(x):(y))
#define max(x,y)	((x)>(y)?(x):(y))
#endif

void lowercase(char *str);
void crash(const char *msg);
bool fileExists(const char *name);
s32 clamp(s32 val, s32 min, s32 max);
void dbgWaitButton(void);
void printMallInfo(void);
int mysqrt(int x);
int b64decode(const char *code, char **res);
void b64encode(u8 *indata, u32 insize, char **outdata);

#endif
