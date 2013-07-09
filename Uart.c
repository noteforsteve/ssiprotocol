#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Portable.h"
#include "Common.h"
#include "Uart.h"

#if OS_TYPE == OS_TYPE_WINDOWS
#include "WinUart.h"
#elif OS_TYPE == OS_TYPE_OSX
#include "OsxUart.h"
#endif

#define DEBUG_MODULE 
//#define DEBUG_LEVEL iDBG_TRACE|DBG_WARN|DBG_ERROR
#define DEBUG_LEVEL DBG_WARN|DBG_ERROR
#include "Debug.h"

#if OS_TYPE == OS_TYPE_WINDOWS
#pragma warning(disable: 4127)
#endif

int
UartCtor(
    OUT uhandle_t	*phUart
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = phUart ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

#if OS_TYPE == OS_TYPE_WINDOWS
	Retval = WinUartCtor(phUart);
#elif OS_TYPE == OS_TYPE_OSX
	Retval = OsxUartCtor(phUart);
#endif

ExitOnFailure:

	return Retval;
}

void
UartDtor(
    IN uhandle_t 	hUart
    )
{
	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    if (hUart)
    {
        UartClose(hUart);

#if OS_TYPE == OS_TYPE_WINDOWS
		WinUartDtor(hUart);
#elif OS_TYPE == OS_TYPE_OSX
		OsxUartDtor(hUart);
#endif
    }
}

int
UartOpen(
    IN uhandle_t 	hUart,
    IN const char   *pName,
    IN unsigned int uRate,
    IN unsigned int uDataBits,
    IN unsigned int uParity,
    IN unsigned int uStopBits
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = hUart && pName ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

#if OS_TYPE == OS_TYPE_WINDOWS
    Retval = WinUartOpen(hUart, pName, uRate, uDataBits, uParity, uStopBits);
#elif (OS_TYPE == OS_TYPE_OSX)
	Retval = OsxUartOpen(hUart, pName, uRate, uDataBits, uParity, uStopBits);
#endif

ExitOnFailure:

	return Retval;
}

void
UartClose(
	IN uhandle_t	hUart
    )
{
	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

#if OS_TYPE == OS_TYPE_WINDOWS
	WinUartClose(hUart);
#elif (OS_TYPE == OS_TYPE_OSX)
	OsxUartClose(hUart);
#endif
}

int
UartRead(
	IN uhandle_t	hUart,
    IO void         *pBuff, 
    IN unsigned int uLength, 
    OUT unsigned int *puRead,
    IN unsigned int uWaitTime
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = hUart && pBuff ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

#if OS_TYPE == OS_TYPE_WINDOWS
   	Retval = WinUartRead(hUart, pBuff, uLength, puRead, uWaitTime);
#elif (OS_TYPE == OS_TYPE_OSX)
	Retval = OsxUartRead(hUart, pBuff, uLength, puRead, uWaitTime);
#endif

ExitOnFailure:

	return Retval;
}

int
UartWrite(
	IN uhandle_t	hUart,
    OUT const void  *pBuff,
    IN unsigned int uLength,
    OUT unsigned int *puWritten,
    IN unsigned int uWaitTime
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = hUart && pBuff ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

#if OS_TYPE == OS_TYPE_WINDOWS
	Retval = WinUartWrite(hUart, pBuff, uLength, puWritten, uWaitTime);
#elif (OS_TYPE == OS_TYPE_OSX)
	Retval = OsxUartWrite(hUart, pBuff, uLength, puWritten, uWaitTime);
#endif

ExitOnFailure:

	return Retval;
}

int
UartSetStatus(
	IN uhandle_t	hUart,
    IN unsigned int uState
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = hUart ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

#if OS_TYPE == OS_TYPE_WINDOWS
   	Retval = WinUartSetStatus(hUart, uState);
#elif (OS_TYPE == OS_TYPE_OSX)
 	Retval = OsxUartSetStatus(hUart, uState);
#endif

ExitOnFailure:

	return Retval;
}

int
UartGetStatus(
	IN uhandle_t	hUart,
    OUT unsigned int *puState
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = hUart && puState ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

#if OS_TYPE == OS_TYPE_WINDOWS
    Retval = WinUartGetStatus(hUart, puState);
#elif (OS_TYPE == OS_TYPE_OSX)
    Retval = OsxUartGetStatus(hUart, puState);
#endif

ExitOnFailure:

	return Retval;
}


/*** Uart Test ****************************************************************/
#if defined (UART_TESTS)

void
UartTestShowModemStatus(
	IN unsigned int uModemStatus
	)
{
	printf("ModemStatus %04x - ", uModemStatus);

	if (uModemStatus & UART_STATUS_CTS)
		printf(" cts ");
	if (uModemStatus & UART_STATUS_DSR)
		printf(" dsr ");
	if (uModemStatus & UART_STATUS_RI)
		printf(" ri ");
	if (uModemStatus & UART_STATUS_DCD)
		printf(" dcd ");
	printf("\n");
}

void
UartTestToUpper(
	IN char 		*p,
	IN unsigned int uLen
	)
{
	for ( ; uLen; uLen = uLen - 1)
	{
		*p = (char)toupper(*p);
		p = p + 1;
	}
}

/* Show state of modem status signals */ 
int 
UartTest(
	IN const char *pszPort
	)
{
	int Retval;
	uhandle_t hUart;
	unsigned int uStatus;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

	Retval = UartCtor(&hUart);
    CHECK_RETVAL(Retval, ExitOnFailure);

	Retval = UartOpen(hUart, 
					  pszPort,
					  UART_RATE_57600, 
					  UART_DATA_BITS_8, 
					  UART_PARITY_NONE, 
					  UART_STOP_1);
    CHECK_RETVAL(Retval, ExitOnFailure);

	for ( ; ; )
	{
		Retval = UartGetStatus(hUart, &uStatus);
    	CHECK_RETVAL(Retval, ExitOnFailure);

		UartTestShowModemStatus(uStatus);

		PortableSleep(1000);
	}
  
ExitOnFailure:

	UartDtor(hUart);

	return Retval;
}

/* Echo back recevied data in upper case */
int 
UartTest1(
	IN const char *pszPort
	)
{
	int Retval;
	uhandle_t hUart;
	char Buff[256];
	unsigned int BytesRead;
	unsigned int BytesWritten;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

	Retval = UartCtor(&hUart);
    CHECK_RETVAL(Retval, ExitOnFailure);

	Retval = UartOpen(hUart, 
					  pszPort,
					  UART_RATE_57600, 
					  UART_DATA_BITS_8, 
					  UART_PARITY_NONE, 
					  UART_STOP_1);
    CHECK_RETVAL(Retval, ExitOnFailure);

	for ( ; ; )
	{
		Retval = UartRead(hUart, Buff, sizeof(Buff), &BytesRead, 1000);
	
		if (BytesRead)
		{	
			UartTestToUpper(Buff, BytesRead);

			Retval = UartWrite(hUart, Buff, BytesRead, &BytesWritten, 1000);
    		CHECK_RETVAL(Retval, ExitOnFailure);
		}
	}
  
ExitOnFailure:

	UartDtor(hUart);

	return Retval;
}

#endif


