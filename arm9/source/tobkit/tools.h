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
#include <fat.h>

#define min(x,y)	((x)<(y)?(x):(y))
#define max(x,y)	((x)>(y)?(x):(y))

void lowercase(char *str);
void crash(const char *msg);
void *mymalloc(size_t size);
void myfree(void *ptr);
void start_malloc_invariant(void);
void end_malloc_invariant(void);
void *mymemalign(size_t blocksize, size_t bytes);
void *my_memset(void *s, int c, u32 n);
char *my_strncpy(char *dest, const char *src, u32 n);
bool myInitFiles(void);
u32 myfread_aligned(void* buffer, u32 size, u32 count, FILE* file);
u32 myfwrite_buffered(const void* buffer, u32 size, u32 count, FILE* file);
bool myfclose_buffered(FILE* file);
bool fileExists(const char *name);
s32 clamp(s32 val, s32 min, s32 max);

void dbgWaitButton(void);

void PrintFreeMem(void);
void printMallInfo(void);

int mysqrt(int x);

int b64decode(const char *code, char **res);
void b64encode(u8 *indata, u32 insize, char **outdata);

#endif
