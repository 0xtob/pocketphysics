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

#include "tools.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <nds.h>

s32 unfreed_malloc_calls = 0;

#define WRITEBUFFER_SIZE	512

#ifdef ARM9

u8 writebuffer[WRITEBUFFER_SIZE] = {0};
u32 writebuffer_pos = 0;
u32 remaining_bytes = 0;

bool fat_inited = false;

// Helper for converting a string to lowercase
void lowercase(char *str)
{
	for(u8 i=0;i<strlen(str);++i) {
		if((str[i]>=65)&&(str[i]<=90)) {
			str[i]+=32;
		}
	}
}

void crash(const char *msg)
{
	iprintf(msg);
	iprintf("Crash! Please send this output  to: me@tobw.net. Sorry :-/\n");
	videoSetMode(MODE_5_2D | DISPLAY_BG0_ACTIVE);
	while(1);
}

void *mymalloc(size_t size)
{
	void *ptr = malloc(size);
	if(ptr!=0) {
		unfreed_malloc_calls++;
	}
	return ptr;
}

void myfree(void *ptr)
{
	if(ptr!=0) {
		unfreed_malloc_calls--;
		free(ptr);
	} else {
		crash("Nullpointer free detected!\n");
	}
}

void start_malloc_invariant(void)
{
	unfreed_malloc_calls = 0;
}

void end_malloc_invariant(void)
{
	if(unfreed_malloc_calls != 0) {
		char err[100] = {0};
		sprintf(err, "Allocation error! Unfreed mallocs: %d\n", unfreed_malloc_calls);
		crash(err);
	}
}

void *mymemalign(size_t blocksize, size_t bytes)
{
	void *buf = memalign(blocksize, bytes);
	if((u32)buf & blocksize != 0) {
		char err[100] = {0};
		sprintf(err, "Memalign error! %p ist not %u-aligned\n", buf, (u32)blocksize);
		crash(err);
		return 0;
	} else {
		unfreed_malloc_calls++;
		return buf;
	}
}

bool myInitFiles(void)
{
	if(!fat_inited) {
		iprintf("FAT init\n");
		fat_inited = true;
		return fatInitDefault();
	}
	else
		return true;
}

#endif

void *my_memset(void *s, int c, u32 n)
{
	u8 *t = (u8*)s;
	u32 i;
	for(i=0; i<n; ++i) {
		t[i] = c;
	}
	return s;
}

char *my_strncpy(char *dest, const char *src, u32 n)
{
	u32 i=0;
	while((src[i] != 0) && (i < n)) {
		dest[i] = src[i];
		i++;
	}
	if((i<n)&&(src[i]==0)) {
		dest[i] = 0;
	}
	return dest;
}

#ifdef ARM9

// DEPRECATED! Reads into a 2-byte aligned buffer
u32 myfread_aligned(void* buffer, u32 size, u32 count, FILE* file)
{
	/*
	// is the buffer aligned?
	if(((u32)buffer & 0x01) != 0) {
		crash("Cannot read aligned into unaligned buffer!\n");
	}
	
	// is filepos even?
	if((ftell(file) & 0x01) == 0) {
		// read and return
		return fread(buffer, size, count, file);
	} else {
		
		u32 actual_size = size*count;
		
		if(actual_size == 0)
			return 0;
		
		// Jump back one byte
		fseek(file, -1, SEEK_CUR);
		
		// Read 2 byte into a 2 byte variable
		u16 start;
		fread(&start, 2, 1, file);
		
		// Read n-1 bytes into the buffer
		u32 readbytes;
		readbytes = fread(buffer, 1, actual_size-1, file);
		
		if(readbytes > actual_size-1) {
			char err[100] = {0};
			sprintf(err, "fread error: too many bytes read: %u of %u\n", readbytes, actual_size+1);
			crash(err);
		}
		else if(readbytes < actual_size-1) {
			char err[100] = {0};
			sprintf(err, "fread error: not enough bytes read: %u of %u\n", readbytes, actual_size-1);
			crash(err);
		}
		
		// Move the buffer one byte back
		memmove((u8*)buffer+1, (u8*)buffer, actual_size-1);
		
		memcpy(buffer, ((u8*)&start)+1, 1);
		
		return actual_size;
	}
	*/
	return fread(buffer, size, count, file);
}

/*
u32 myfwrite_buffered(const void* buffer, u32 size, u32 count, FILE* file)
{
	u32 bytes = size*count;
	
	writebuffer = realloc(writebuffer, writebuffer_size + bytes);
	if(writebuffer==0) crash("Mem full!\n");
	memcpy((u8*)writebuffer+writebuffer_size, buffer, bytes);
	
	writebuffer_size += bytes;
	
	return size*count;
}
*/
/*
u32 myfwrite_buffered(const void* buffer, u32 size, u32 count, FILE* file)
{
	u32 bytes = size*count;
	
	u32 writebuffer_rest = WRITEBUFFER_SIZE - writebuffer_pos;
	
	if(bytes < writebuffer_rest) {
		
		// If the data still fits into the buffer
		// just copy it there.
		
		memcpy((u8*)writebuffer+writebuffer_pos, buffer, bytes);
		writebuffer_pos += bytes;
		remaining_bytes += bytes;
		
	} else {
		
		// If the buffer is filled
		remaining_bytes += bytes;
		
		u32 bytes_to write = writebuffer_rest;
		
		while(remaining_bytes > WRITEBUFFER_SIZE) {
			
			memcpy((u8*)writebuffer+writebuffer_pos, buffer, writebuffer_rest);
			
			fwrite(writebuffer, WRITEBUFFER_SIZE, 1, file);
			
			writebuffer_pos = 0;
			remaining_bytes -= WRITEBUFFER_SIZE;
			buffer += writebuffer_rest;
			writebuffer_rest = WRITEBUFFER_SIZE;
			
		}
		
		memcpy((u8*)writebuffer, buffer, remaining_bytes);
		writebuffer_pos = remaining_bytes;
		
	}
	
	return bytes;
}

bool myfclose (FILE* file)
{
	if(remaining_bytes > 0) {
		// Flush the cache
		fwrite(writebuffer, remaining_bytes, 1, file);
		
	}
}
*/

u32 myfwrite_buffered(const void* buffer, u32 size, u32 count, FILE* file)
{
	u32 buffer_rest = WRITEBUFFER_SIZE - writebuffer_pos;
	u32 bytes = size * count;
	
	if(bytes < buffer_rest) {
		
		memcpy(writebuffer+writebuffer_pos, buffer, bytes);
		writebuffer_pos += bytes;
		
	} else {
		
		memcpy(writebuffer+writebuffer_pos, buffer, buffer_rest);
		buffer = (u8*)buffer + buffer_rest;
		
		// Flush
		fwrite(writebuffer, WRITEBUFFER_SIZE, 1, file);
		writebuffer_pos = 0;
		
		bytes -= buffer_rest;
		buffer_rest = WRITEBUFFER_SIZE;
		
		while(bytes >= WRITEBUFFER_SIZE) {
			
			memcpy(writebuffer, buffer, WRITEBUFFER_SIZE);
			buffer = (u8*)buffer + WRITEBUFFER_SIZE;
			bytes -= WRITEBUFFER_SIZE;
			
			// Flush
			fwrite(writebuffer, WRITEBUFFER_SIZE, 1, file);
			writebuffer_pos = 0;
			
		}
		
		memcpy(writebuffer, buffer, bytes);
		writebuffer_pos += bytes;
		
	}
	
	return size*count;
}


bool myfclose_buffered(FILE* file)
{
	fwrite(writebuffer, writebuffer_pos, 1, file);
	writebuffer_pos = 0;
	
	return fclose(file);
}


/*
bool myfclose (FILE* file)
{
	if(writebuffer!=0) { // We are in write mode
		// Flush the write buffer
		fwrite(writebuffer, writebuffer_size, 1, file);
		
		free(writebuffer);
		writebuffer = 0;
		writebuffer_size = 0;
	}
	
	return fclose(file);
}
*/

void dbgWaitButton(void)
{
	iprintf("press the any key...\n");
	scanKeys();
	while(! (keysDown()&KEY_A) ) scanKeys();
}

bool fileExists(const char *filename)
{
	myInitFiles();
	
	bool res;
	FILE* f = fopen(filename,"r");
	if(f == NULL) {
		res = false;
	} else {
		fclose(f);
		res = true;
	}
	
	return res;
}

#endif

s32 clamp(s32 val, s32 min, s32 max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

// Borrowed from Infantile Paralyser
bool testmalloc(int size)
{
	if(size<=0) return(false);
	
	void *ptr;
	u32 adr;
	
	ptr=malloc(size+(64*1024)); // 64kb
	
	if(ptr==NULL) return(false);
	
	adr=(u32)ptr;
	free(ptr);
	
	if((adr&3)!=0){ // 4byte
		return(false);
	}
	
	if((adr+size)<0x02000000){
		return(false);
	}
	
	if((0x02000000+(4*1024*1024))<=adr){
		return(false);
	}
	
	return(true);
}

#define PrintFreeMem_Seg (1024)

// Borrowed from Infantile Paralyser
void PrintFreeMem(void)
{
	s32 i;
	u32 FreeMemSize=0;
	
	for(i=1*PrintFreeMem_Seg;i<4096*1024;i+=PrintFreeMem_Seg){
		if(testmalloc(i)==false) break;
		FreeMemSize=i;
	}
	
	iprintf("FreeMem=%dbyte    \n",FreeMemSize);
}

void printMallInfo(void)
{
	struct mallinfo mi = mallinfo();
	iprintf("non-inuse chunks: %d\n", mi.fordblks);
	iprintf("sbrk bytes:       %d\n", mi.arena);
	iprintf("mmap bytes:       %d\n", mi.hblkhd);
	iprintf("malloc chunks:    %d\n", mi.uordblks);
}
