#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*** Windows **************************************************************/

#if defined(WINDOWS)

#include <Windows.h>
#include <conio.h>

#include "Portable.h"

unsigned int 
PortableGetTick(
    void
    )
{
    return GetTickCount();
}

void
PortableSleep(
    unsigned int t
    )
{
    Sleep(t);
}

int 
PortableKeyRead(
	void
	)
{
	return _getch();
}

int 
PortableKeyHit(
	void
	)
{
	return _kbhit();
}

#endif

/*** OSX *********************************************************************/

#if defined(OSX)

#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "Portable.h"

unsigned int 
PortableGetTick(
	void
	)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (int)(tv.tv_sec*1000 + (tv.tv_usec / 1000));
}

void
PortableSleep(
    unsigned int t
    )
{
	usleep(t*1000);
}

int 
PortableKeyWait(
	int 			*pch,
	unsigned int 	uMaxWait
	)
{
	int Retval;
	struct termios term;
	struct termios oldt;
    int bytesWaiting;
	unsigned int uStartTick;

	/* Get current termio state */
	tcgetattr(STDIN_FILENO, &term);
	oldt = term;

	/* Disable blocking and echo */
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

	/* Get the start time */
	uStartTick = PortableGetTick();

	for ( ; ; )
	{
		/* Check if characters waiting in stdin */
    	ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
	
		if (bytesWaiting)
		{
			*pch = getchar();
			Retval = 0;
			break;
		}
	
		if (uMaxWait != 0xFFFFFFFF)
		{	
			/* Has the max time elappsed */
			if (PortableGetTick() - uStartTick > uMaxWait)
			{
				Retval = -1;
				break;
			}
		}
	}
  
	/* Restore the termio settings */ 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	return Retval;
}


#endif

