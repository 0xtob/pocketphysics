/*
 *    Pocket Physics - A mechanical construction kit for Nintendo DS
 *                   Copyright 2005-2010 Tobias Weyand (me@tobw.net)
 *                            http://code.google.com/p/pocketphysics
 *
 * TobKit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * TobKit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Pocket Physics. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "tools.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <nds.h>

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char dec64table[] = {
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*  0-15 */
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /* 16-31 */
  255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63, /* 32-47 */
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,255,255,255, /* 48-63 */
  255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 64-79 */
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255, /* 80-95 */
  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 96-111 */
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255  /* 112-127*/
};

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

void dbgWaitButton(void)
{
	iprintf("press the any key...\n");
	scanKeys();
	while(! (keysDown()&KEY_A) ) scanKeys();
}

bool fileExists(const char *filename)
{
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

s32 clamp(s32 val, s32 min, s32 max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

void printMallInfo(void)
{
	struct mallinfo mi = mallinfo();
	iprintf("non-inuse chunks: %d\n", mi.fordblks);
	iprintf("sbrk bytes:       %d\n", mi.arena);
	iprintf("mmap bytes:       %d\n", mi.hblkhd);
	iprintf("malloc chunks:    %d\n", mi.uordblks);
}

int mysqrt(int x)
{
    unsigned long long m, root = 0, left = (unsigned long long)x;
    for ( m = (long long)1<<( (sizeof(long long)<<3) - 2); m; m >>= 2 )
    {
        if ( ( left & -m ) > root )
            left -= ( root += m ), root += m;
        root >>= 1;
    }
    return root;
}

int b64decode(const char *code, char **res)
{
	int x, y;
	char *result = (char*)calloc(1, 3*(strlen(code)/4) + 1);

	*res = result;

	// Each cycle of the loop handles a quantum of 4 input bytes. For the last
	// quantum this may decode to 1, 2, or 3 output bytes.

	while ((x = (*code++)) != 0)
	{
		if (x > 127 || (x = dec64table[x]) == 255)
			return -1;
		if ((y = (*code++)) == 0 || (y = dec64table[y]) == 255)
			return -1;
		*result++ = (x << 2) | (y >> 4);

		if ((x = (*code++)) == '=')
		{
			if (*code++ != '=' || *code != 0)
				return -1;
		}
		else
		{
			if (x > 127 || (x = dec64table[x]) == 255)
				return -1;
			*result++ = (y << 4) | (x >> 2);
			if ((y = (*code++)) == '=')
			{
				if (*code != 0) return -1;
			}
			else
			{
				if (y > 127 || (y = dec64table[y]) == 255)
					return -1;
				*result++ = (x << 6) | y;
			}
		}
	}

	*result = 0;
	return result - *res;
}

// base64 encode a stream adding padding and line breaks as per spec.
void b64encodeblock(unsigned char in[3], unsigned char out[4], int len)
{
	out[0] = cb64[ in[0] >> 2 ];
	out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
	out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
	out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

// base64 encode a stream adding padding and line breaks as per spec.
// modified by tob to use buffers instead of files and not use linewidth
void b64encode(u8 *indata, u32 insize, char **outdata)
{
	unsigned char in[3], out[4];
	int i, len, blocksout = 0;
	u32 inpos = 0;
	u32 outpos = 0;
	
	u8 modulo = 3-insize%3;
	u32 outsize = (insize+modulo) * 4 / 3;
	char *outptr = (char*)malloc( outsize + 1 );
	*outdata = outptr;
	memset(outptr, 0, outsize + 1);
	
	while( inpos < insize ) {
		len = 0;
		for( i = 0; i < 3; i++ ) {
			in[i] = (unsigned char)indata[inpos];
			inpos++;
			if( inpos <= insize ) {
					len++;
			} else {
					in[i] = 0;
			}
		}
		if( len ) {
			b64encodeblock( in, out, len );
			for( i = 0; i < 4; i++ ) {
				//putc( out[i], outfile );
				outptr[outpos] = out[i];
				outpos++;
			}
			blocksout++;
		}
		if( inpos >= insize ) {
			if( blocksout ) {
				//fprintf( outfile, "\r\n" );
				//sprintf( (outptr+outpos), "\r\n");
			}
			blocksout = 0;
		}
	}
}
