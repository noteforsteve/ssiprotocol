#include <stdio.h>
#include <string.h>
#include "debug.h"

void 
DebugDump(
    size_t      addr,
    const void  *p, 
    int         len
    )
{
    const char *s = p;
    int i;
    int size;

    for ( ; len; )
    {
        fprintf(stderr, "%p: ", (void *)addr);
        
        size = len > 16 ? 16 : len;

        for (i = 0; i < 16; i = i + 1)
        {
            if (i < size)
            {
                fprintf(stderr, "%02x ", s[i] & 0xFF);
            }
            else
            {
                fprintf(stderr, "   ");
            }
        }

        for (i = 0; i < 16; i = i + 1)
        {
            if (i < size)
            {
                fprintf(stderr, "%c", (s[i] > ' ' && s[i] <= 'z') ? s[i] : '.');
            }
            else
            {
                fprintf(stderr, " ");
            }
        }

        fprintf(stderr, "\n");

        len = len - size;
        s = s + size;
        addr = addr + size;
    }
}

const char *
DebugTrimFileName(
	const char *pszFile
	)
{
	const char *pszStart = "";
	int uLen;
    int i;
	
	if (pszFile)
	{
		uLen = strlen(pszFile);
		
		if (uLen)
		{
			pszStart = pszFile;
			
			for (i = uLen - 1; i; i--)
			{ 
				if (pszFile[i] == '\\')
				{
					pszStart = &pszFile[i+1];
					break;
				}					
			}
		}
	}
	
	return pszStart;
}											
