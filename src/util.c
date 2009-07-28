#include <stdlib.h>
#include <assert.h>
#include "os.h"

#include "util.h"

static int __tolower(char c)
{
	if('A' <= c && c <= 'Z')
		return c - 'A' + 'a';
	return c;
}

static INT hexchar_to_int(CHAR c)
{
	if('0' <= c && c <= '9')
		return c - '0';
	if('a' <= __tolower(c) && __tolower(c) <= 'f')
		return __tolower(c) - 'a' + 10;
	
	assert(NULL);
	return 0;
}

static CHAR int_to_hexchar(INT h)
{
	assert(h >= 0 && h <= 15);
	if(h<=9) return h + '0';
	return h + 'a' - 10;
}

BYTE *hex2buf(CHAR *szStr, BYTE *bytes, INT *len)
{
	INT bFirstChar = 1;
	CHAR lastChar = 0;
	BYTE *oldbytes;
	
	oldbytes = bytes;

	if(len) *len = 0;

	for(;*szStr; szStr++)
	{
		if(('0' <= *szStr && *szStr <= '9') || ('a' <= __tolower(*szStr) && __tolower(*szStr) <= 'f'))
		{
			if(bFirstChar)
			{
				lastChar = *szStr;
			}
			else
			{
				*bytes = hexchar_to_int(lastChar) * 0x10 + hexchar_to_int(*szStr);
				bytes++;
				if(len) (*len)++;
			}
			bFirstChar = !bFirstChar;
		}
	}
	return oldbytes;
}

CHAR *buf2hex(BYTE *bytes, INT len , CHAR *szStr)
{
	INT i;
	CHAR *oldstr;
	oldstr = szStr;
	for(i=0;i<len;i++,bytes++)
	{
		*szStr = int_to_hexchar((*bytes & 0xF0) / 0x10);
		szStr++;
		*szStr = int_to_hexchar(*bytes & 0xF);
		szStr++;
		//*szStr = ' ';
		//szStr++;
	}
	*szStr = 0;
	return oldstr;
}

CHAR *buf2output(BYTE *bytes, INT len , CHAR *szStr, INT linelen)
{
	INT i;
	CHAR *oldstr;
	oldstr = szStr;

	for(i=0;i<len;i++,bytes++)
	{
		*szStr = int_to_hexchar((*bytes & 0xF0) / 0x10);
		szStr++;
		*szStr = int_to_hexchar(*bytes & 0xF);
		szStr++;
		*szStr = ' ';
		szStr++;

		if( (i + 1) % linelen == 0)
		{
			*szStr = '\n';
			szStr++;
		}
	}
	*szStr = 0;
	return oldstr;
}
