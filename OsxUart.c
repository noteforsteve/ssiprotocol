#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>                                         
#include <fcntl.h>                                                
#include <errno.h>                                                 
#include <termios.h>                                       
#include <sys/ioctl.h>
#include <time.h>

#include "Common.h"
#include "Uart.h"
#include "Portable.h"
#include "OsxUart.h"

#define DEBUG_MODULE
// #define DEBUG_LEVEL DBG_TRACE|DBG_WARN|DBG_ERROR 
#define DEBUG_LEVEL DBG_ERROR 
#include "Debug.h"

/*** Type Definitions ********************************************************/

typedef struct                                                                                      
{                                                                                                   
    unsigned int           uNormal;                                                                 
    unsigned int           uSpecific;                                                               
} OsxEntry_T;                                                                                       
                                                                                                    
const OsxEntry_T gBaudTable [] =                                                                    
{                                                                                                   
    {UART_RATE_9600,    B9600},                                                                  
    {UART_RATE_14400,   B14400},                                                                 
    {UART_RATE_19200,   B19200},                                                                 
    {UART_RATE_38400,   B38400},                                                                 
    {UART_RATE_57600,   B57600},                                                                 
    {UART_RATE_115200,  B115200},                                                                
};                                                                                                  

const OsxEntry_T gDataBitsTable [] =                                                                
{                                                                                                   
    {UART_DATA_BITS_7,  CS7},                                                                         
    {UART_DATA_BITS_8,  CS8},                                                                         
};                                                                                                  
                                                                                                    
const OsxEntry_T gParityTable [] =                                                                  
{                                                                                                   
    {UART_PARITY_NONE,  0},                                                                  
    {UART_PARITY_ODD,   PARENB|PARODD},                                                                 
    {UART_PARITY_EVEN,  PARENB},                                                                
};                                                                                                  
                                                                                                    
const OsxEntry_T gStopBitsTable [] =                                                                
{                                                                                                   
    {UART_STOP_1,       0},                                                                
    {UART_STOP_2,       CSTOPB},                                                               
};

typedef struct
{
	int 			hPort;
    struct termios  PortOptions;                                                                         
} OsxUart_T;

/*** Function Prototypes *****************************************************/

int
OsxUartLookup(
    unsigned int        uNormal,
    const OsxEntry_T    *pEntry,
    int                 uCount,
    unsigned int        *puSpecific
    );

int
OsxUartConvertSettings(
    unsigned int *puRate, 
    unsigned int *puBits, 
    unsigned int *puParity, 
    unsigned int *puStop
    );

/*** Public Functions ********************************************************/

int
OsxUartCtor(
	IN uhandle_t	*phUart
    )
{   
    int Retval;                                                                                     
                                                                                                    
    DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);                                                       
                                                                                                    
    *phUart = (uhandle_t)malloc(sizeof(OsxUart_T));                                                                
    Retval = *phUart ? S_OK : E_NOMEMORY;                                                               
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            

    memset((void *)*phUart, 0, sizeof(OsxUart_T));                                                              
                                                                                                    
ExitOnFailure:                                                                                      
                                                                                                    
    return Retval; 
}

void
OsxUartDtor(
	IN uhandle_t	hUart
    )
{
	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);
	free ((void *)hUart);
}

int
OsxUartOpen(
	IN uhandle_t	hUart,
    IN const char   *pName,
    IN unsigned int uRate,
    IN unsigned int uBits,
    IN unsigned int uParity,
    IN unsigned int uStop
    )
{
	int Retval;
	OsxUart_T *pUart = (OsxUart_T *)hUart;
    struct termios options;                                                                         

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

	/* Open the port non blocking */
    pUart->hPort = open(pName, O_RDWR | O_NOCTTY | O_NDELAY);                                            
	Retval = pUart->hPort != -1 ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            

    /* 
	 * Note: open() follows POSIX semantics: multiple open() calls to the same file will 
	 * succeed unless the TIOCEXCL ioctl is issued. This will prevent additional opens except 
	 * by root-owned processes.      
	 */                                                                             
    Retval = ioctl(pUart->hPort, TIOCEXCL) != -1 ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            
	
    /* Get the current port settings */
    Retval = tcgetattr(pUart->hPort, &options);                                                                        
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            

	/* Make a copy of the port options, restored on close */
	pUart->PortOptions = options;

	/* Clear parity, stop bit and bit */
	options.c_cflag = options.c_cflag & ~(PARENB | PARODD | CSTOPB);
	
	/* Convert the universal settings to the specific settings */
	Retval = OsxUartConvertSettings(&uRate, &uBits, &uParity, &uStop);
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            

    // Set the baud rates                                                                
    Retval = cfsetspeed(&options, uRate);                                                                 
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            

    /* set the bits per character */
    options.c_cflag &= ~CSIZE;                                                                  
    options.c_cflag |= uBits;                                                                         

    // set the parity 
    options.c_cflag &= ~(PARENB | PARODD);                                                                     
	options.c_cflag |= uParity;

    // set the stop bits 
    options.c_cflag &= ~CSTOPB;                                                                 
	options.c_cflag |= uStop;

    // no hw flowcontrol                                                                     
    options.c_cflag &= ~CRTSCTS;                                                                

    // ignore modem controllines + enable receiver                                                   
    options.c_cflag |= (CLOCAL | CREAD);                                                            
                                                                                                    
	// raw mode                                                                                  
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);                                         

   	// no sw flowcontrol                                                                         
    options.c_lflag &= ~(IXON | IXOFF | IXANY);                                                 
                                                                                                    
    // raw mode output                                                                               
    options.c_oflag &= ~OPOST;                                                                  

    // non blocking                                                                                  
    options.c_cc[VMIN]=0;                                                                           
    options.c_cc[VTIME]=0;                                                                          

    // Set the new options for the port...                                                          
    Retval = tcsetattr(pUart->hPort, TCSANOW|TCSAFLUSH, &options);                                                     
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            
                                                                                                    
ExitOnFailure:

    return Retval;
}

void
OsxUartClose(
	IN uhandle_t	hUart
    )
{
	int Retval;
    OsxUart_T *pUart = (OsxUart_T *)hUart;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    if (pUart && pUart->hPort)
    {
		/* Restore the port settings */
    	Retval = tcsetattr(pUart->hPort, 
						   TCSANOW|TCSAFLUSH, 
						   &pUart->PortOptions);                                                     
		
        close(pUart->hPort);

        pUart->hPort = 0;
    }
}

int
OsxUartRead(
	IN uhandle_t	hUart,
    IO void         *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puRead,   
    IN unsigned int uWaitTime   
    )
{
	int Retval;
	OsxUart_T *pUart = (OsxUart_T *)hUart;
	int iRead;
	unsigned int uTotal;
	unsigned int uStart;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

	uTotal = 0;
	uStart = PortableGetTick();

	for ( ; ; )
	{
		iRead = read(pUart->hPort, pBuff+uTotal, uLength-uTotal);
		Retval = iRead != -1 ? S_OK : S_OK;
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            

		uTotal = uTotal + iRead;

		if (uTotal == uLength)
		{
			break;
		}

		if (PortableGetTick() - uStart > uWaitTime)
		{
			Retval = (uTotal) ? S_OK : E_TIMEOUT;
			break;
		}

		PortableSleep(100);
	}

	if (puRead)
	{
		*puRead = uTotal;
	}

ExitOnFailure:

	return Retval;
}

int
OsxUartWrite(
	IN uhandle_t	hUart,
    IO const void   *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puWritten,   
    IN unsigned int uWaitTime   
    )
{
	int Retval;
	OsxUart_T *pUart = (OsxUart_T *)hUart;
	size_t Written;
	unsigned int uTotal;
	unsigned int uStart;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

	uTotal = 0;
	uStart = PortableGetTick();

	for ( ; ; )
	{
		Written = write(pUart->hPort, pBuff+uTotal, uLength-uTotal);
		Retval = Written != -1 ? S_OK : E_FAIL;
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            

		uTotal = uTotal + Written;

		if (uTotal == uLength)
		{
			Retval = S_OK;
			break;
		}

		if (PortableGetTick() - uStart > uWaitTime)
		{
			Retval = uTotal ? S_OK : E_TIMEOUT;
			break;
		}
	
		PortableSleep(100);
	}

	if (puWritten)
	{
		*puWritten = uTotal;
	}

ExitOnFailure:

   	return Retval;
}

int
OsxUartSetStatus(
	IN uhandle_t	hUart,
    IN unsigned int uState
    )
{
	int Retval;
	OsxUart_T *pUart = (OsxUart_T *)hUart;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

	Retval = pUart ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            

	switch(uState)
	{
	case UART_STATUS_SETRTS:
    	Retval = ioctl(pUart->hPort, TIOCMGET, &uState);                                                              
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            
		uState = uState | TIOCM_RTS;
    	Retval = ioctl(pUart->hPort, TIOCMSET, &uState);                                                              
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            
		break;

	case UART_STATUS_CLRRTS:                                                                       
    	Retval = ioctl(pUart->hPort, TIOCMGET, &uState);                                                              
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            
		uState = uState & ~TIOCM_RTS;
    	Retval = ioctl(pUart->hPort, TIOCMSET, &uState);                                                              
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            
		break;

	case UART_STATUS_SETDTR:                                                                       
		uState = 0;
    	Retval = ioctl(pUart->hPort, TIOCSDTR, &uState);                                                              
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            
		break;

	case UART_STATUS_CLRDTR: 
		uState = 0;
    	Retval = ioctl(pUart->hPort, TIOCCDTR, &uState);                                                              
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            
		break;

	default:
		Retval = E_INVALIDARG;
    	CHECK_RETVAL(Retval, ExitOnFailure);                                                            
		break;
	}

ExitOnFailure:
                                                                                                    
    return Retval;
}

int
OsxUartGetStatus(
	IN uhandle_t	hUart,
    OUT unsigned int *puState
    )
{
	int Retval;
	int iStatus;
	OsxUart_T *pUart = (OsxUart_T *)hUart;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

	Retval = pUart && puState ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            
                                                                                                    
    Retval = ioctl(pUart->hPort, TIOCMGET, &iStatus);                                                               
    CHECK_RETVAL(Retval, ExitOnFailure);                                                            

	*puState = 0;

	if (iStatus & TIOCM_DSR)
	{
		*puState = *puState | UART_STATUS_DSR;
	}

	if (iStatus & TIOCM_CTS) 
	{                                                                     
		*puState = *puState | UART_STATUS_CTS;
	}

	if (iStatus & TIOCM_RNG) 
	{                                                                    
		*puState = *puState | UART_STATUS_RI;
	}

	if (iStatus & TIOCM_CD) 
	{                                                                      
		*puState = *puState | UART_STATUS_DCD;
	}

ExitOnFailure:

    return Retval;
}


/*** Private Functions ********************************************************/

int
OsxUartLookup(
    unsigned int        uNormal,
    const OsxEntry_T    *pEntry,
    int                 uCount,
    unsigned int        *puSpecific
    )
{
    int Retval = E_FAIL;
    int i;

	DBG_MSG(DBG_NONE, "%s\n", __FUNCTION__);

    *puSpecific = 0;

    for (i = 0; i < uCount; i = i + 1)
    {
        if (pEntry[i].uNormal == uNormal)
        {
            *puSpecific = pEntry[i].uSpecific;
            Retval = S_OK;
            break;
        }
    }

    return Retval;
}

int
OsxUartConvertSettings(
    unsigned int *puRate, 
    unsigned int *puBits, 
    unsigned int *puParity, 
    unsigned int *puStop
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = OsxUartLookup(*puRate, gBaudTable, COUNTOF(gBaudTable), puRate);
    CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = OsxUartLookup(*puBits, gDataBitsTable, COUNTOF(gDataBitsTable), puBits);
    CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = OsxUartLookup(*puParity, gParityTable, COUNTOF(gParityTable), puParity);
    CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = OsxUartLookup(*puStop, gStopBitsTable, COUNTOF(gStopBitsTable), puStop);
    CHECK_RETVAL(Retval, ExitOnFailure);

ExitOnFailure:

	return Retval;
}

