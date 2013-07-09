#ifndef PORTABLE_H
#define PORTABLE_H

#define OS_TYPE_UNKNOWN 0
#define OS_TYPE_WINDOWS 1
#define OS_TYPE_OSX     2

#if defined(WINDOWS)
#define OS_TYPE             OS_TYPE_WINDOWS
#define PATH_SEPARATOR      '\\'
#define PATH_MAX_LENGTH     260
#define snprintf            _snprintf
#pragma warning(disable: 4101)
#endif

#if defined(OSX)
#define OS_TYPE             OS_TYPE_OSX
#define PATH_SEPARATOR      '/'
#define PATH_MAX_LENGTH     256
#endif

/*** Portable functions ******************************************************/

unsigned int 
PortableGetTick(
    void
    );

void
PortableSleep(
    unsigned int 
    );

int 
PortableKeyWait(
	int 			*pch,
	unsigned int 	uMaxWait
	);

#endif
