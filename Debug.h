#ifndef DEBUG_H
#define DEBUG_H

#if defined(DEBUG) && defined(DEBUG_MODULE)

#define DBG_NONE    (0 << 0)
#define DBG_TRACE   (1 << 1)
#define DBG_WARN    (1 << 2)
#define DBG_ERROR   (1 << 3)

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_TRACE|DBG_WARN|DBG_ERROR
#endif

#if OS_TYPE == OS_TYPE_WINDOWS
#pragma warning(disable: 4127)
#pragma warning(disable: 4100)
#endif

static int gDebugLevel = DEBUG_LEVEL;

#define DBG_MSG(level, ...)\
        do {\
            if (gDebugLevel & level)\
            {\
                fprintf(stderr, "debug: %s %d: ", DebugTrimFileName(__FILE__), __LINE__);\
                fprintf(stderr, __VA_ARGS__);\
            }\
        } while(0)

#define DBG_DUMP(level, addr, buff, len)\
        do {\
            if (gDebugLevel & level)\
            {\
                DebugDump(addr, (const void *)buff, (int)len);\
            }\
        } while (0)

void 
DebugDump(
    size_t      addr,
    const void  *p, 
    int         len
    );

const char *
DebugTrimFileName(
	const char *pszFile
	);

#endif

#if !defined(DEBUG) || !defined(DEBUG_MODULE)

#define DBG_NONE    
#define DBG_TRACE   
#define DBG_WARN    
#define DBG_ERROR   
#define DBG_MSG(level, ...)
#define DBG_DUMP(level, addr, buff, len)

#endif

#endif
