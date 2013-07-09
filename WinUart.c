#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Portable.h"
#include "Common.h"
#include "Uart.h"
#include "WinUart.h"

#define DEBUG_MODULE
//#define DEBUG_LEVEL DBG_TRACE|DBG_WARN|DBG_ERROR 
#define DEBUG_LEVEL DBG_WARN|DBG_ERROR
#include "Debug.h"

#define DEFAULT_READ_TIMEOUT    100
#define DEFAULT_WRITE_TIMEOUT   100

typedef struct 
{
    unsigned int           uNormal;
    unsigned int           uSpecific;
} WinEntry_T;

const WinEntry_T gBaudTable [] =
{
    {UART_RATE_9600,    CBR_9600},
    {UART_RATE_14400,   CBR_14400},
    {UART_RATE_19200,   CBR_19200},
    {UART_RATE_38400,   CBR_38400},
    {UART_RATE_56000,   CBR_56000},
    {UART_RATE_57600,   CBR_57600},
    {UART_RATE_115200,  CBR_115200},
    {UART_RATE_128000,  CBR_128000},
    {UART_RATE_256000,  CBR_256000},
};

const WinEntry_T gDataBitsTable [] = 
{
    {UART_DATA_BITS_7,  7},
    {UART_DATA_BITS_8,  8},
};

const WinEntry_T gParityTable [] =
{
    {UART_PARITY_NONE,  NOPARITY},
    {UART_PARITY_ODD,   ODDPARITY},
    {UART_PARITY_EVEN,  EVENPARITY},
    {UART_PARITY_MARK,  MARKPARITY},
    {UART_PARITY_SPACE, SPACEPARITY},
};

const WinEntry_T gStopBitsTable [] = 
{
    {UART_STOP_1,       ONESTOPBIT},
    {UART_STOP_1_5,     ONE5STOPBITS},
    {UART_STOP_2,       TWOSTOPBITS},
};

const WinEntry_T gModemStatusTable [] = 
{
    {UART_STATUS_SETRTS,    SETRTS},
    {UART_STATUS_CLRRTS,    CLRRTS},
    {UART_STATUS_SETDTR,    SETDTR},
    {UART_STATUS_CLRDTR,    CLRDTR},
};

typedef struct 
{
    HANDLE      hPort;
} WinUart_T;

/*** Function Prototypes *****************************************************/

int
WinUartLookup(
    unsigned int        uNormal,
    const WinEntry_T    *pEntry,
    int                 uCount,
    unsigned int        *puSpecific
    );

int
WinUartConvertSettings(
    unsigned int *puRate, 
    unsigned int *puBits, 
    unsigned int *puParity, 
    unsigned int *puStop
    );

/*** Public Functions ********************************************************/

int
WinUartCtor(
    OUT uhandle_t   *phUart
    )
{
    int Retval;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    *phUart = (uhandle_t)malloc(sizeof(WinUart_T));
    Retval = *phUart ? S_OK : E_NOMEMORY;
    CHECK_RETVAL(Retval, ExitOnFailure);

    memset((void *)*phUart, 0, sizeof(WinUart_T));
    
ExitOnFailure:

    return Retval;
}

void
WinUartDtor(
    IN uhandle_t    hUart
    )
{
	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);
    free((void *)hUart);
}

int
WinUartOpen(
    IN uhandle_t    hUart,
    IN const char   *pName,
    IN unsigned int uRate,
    IN unsigned int uBits,
    IN unsigned int uParity,
    IN unsigned int uStop
    )
{
    int Retval;
	DCB dcb;
	COMMTIMEOUTS CommTimeouts;
	DWORD dwErrors;
    WinUart_T *pUart = (WinUart_T *)hUart;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    /* Ensure we are in the closed state */
    Retval = pUart->hPort == NULL ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

    /* Normalize the settings */
    Retval = WinUartConvertSettings(&uRate, &uBits, &uParity, &uStop);
    CHECK_RETVAL(Retval, ExitOnFailure);

	DBG_MSG(DBG_TRACE, "Port name %s\n", pName);

	/* Open the serial port */
	pUart->hPort = CreateFile(pName,        // Pointer to the name of the port
                              GENERIC_READ | GENERIC_WRITE, // Access (read-write) mode 
                              0,            // Share mode
		                      NULL,         // Pointer to the security attribute
		                      OPEN_EXISTING,// How to open the serial port
		                      0,            // Port attributes
						      NULL);        // Handle to port with attribute to copy

	/* Did an error occurr */
	Retval = (pUart->hPort != INVALID_HANDLE_VALUE) ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

	/* Purge cannot be called before the port is opened */
    PurgeComm(pUart->hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);

    /* Set the com timeouts */
	CommTimeouts.ReadIntervalTimeout			= 0;
	CommTimeouts.ReadTotalTimeoutConstant		= DEFAULT_READ_TIMEOUT;
	CommTimeouts.ReadTotalTimeoutMultiplier		= 0;
	CommTimeouts.WriteTotalTimeoutConstant		= DEFAULT_WRITE_TIMEOUT;
	CommTimeouts.WriteTotalTimeoutMultiplier	= 0;

    /* Push down the setting */
	Retval = SetCommTimeouts(pUart->hPort, &CommTimeouts) ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

    /* Clear any outstanding errors */
	Retval = ClearCommError(pUart->hPort, &dwErrors, NULL) ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

    /* Get the current com state */
	Retval = GetCommState(pUart->hPort, &dcb) ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

    /* Setup the device control block */
	dcb.BaudRate		= uRate;				/* Baudrate at which running       */
	dcb.fBinary			= TRUE;					/* Binary Mode (skip EOF check)    */
	dcb.fParity			= TRUE;					/* Enable parity checking          */
	dcb.fOutxCtsFlow	= FALSE;				/* CTS handshaking on output       */
	dcb.fOutxDsrFlow	= FALSE;				/* DSR handshaking on output       */
	dcb.fDtrControl		= DTR_CONTROL_DISABLE;	/* DTR Flow control                */
	dcb.fDsrSensitivity = FALSE;				/* DSR Sensitivity				   */
	dcb.fTXContinueOnXoff = FALSE;				/* Continue TX when Xoff sent	   */
	dcb.fOutX			= FALSE;				/* Enable output X-ON/X-OFF        */
	dcb.fInX			= FALSE;				/* Enable input X-ON/X-OFF         */
	dcb.fErrorChar		= FALSE;				/* Enable Err Replacement          */
	dcb.fNull			= FALSE;				/* Enable Null stripping           */
	dcb.fRtsControl		= RTS_CONTROL_DISABLE;  /* Rts Flow control                */
	dcb.fAbortOnError	= FALSE;				/* Abort reads and writes on Error */
	dcb.fDummy2			= 0;					/* Reserved                        */
	dcb.wReserved       = 0;					/* Not currently used              */
	dcb.XonLim          = 0;					/* Transmit X-ON threshold         */
	dcb.XoffLim         = 0;					/* Transmit X-OFF threshold        */
	dcb.ByteSize        = (BYTE)uBits;		    /* Number of bits/byte, 4-8        */
	dcb.Parity          = (BYTE)uParity;		/* 0-4=None,Odd,Even,Mark,Space    */
	dcb.StopBits        = (BYTE)uStop;		    /* 0,1,2 = 1, 1.5, 2               */
	dcb.XonChar			= 0;					/* Tx and Rx X-ON character        */
	dcb.XoffChar		= 0;					/* Tx and Rx X-OFF character       */
	dcb.ErrorChar		= 0;					/* Error replacement char          */
	dcb.EofChar			= 0;					/* End of Input character          */
	dcb.EvtChar			= 0;					/* Received Event character        */
	dcb.wReserved1		= 0;					/* Fill for now.                   */

    // Set the com state, i.e. new rate, settings
	Retval = SetCommState(pUart->hPort, &dcb) ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

    return Retval;

ExitOnFailure:

    // Something failed close the port, if opened
    WinUartClose(hUart);

    return Retval;
}

void
WinUartClose(
    IN uhandle_t    hUart
    )
{
    WinUart_T *pUart = (WinUart_T *)hUart;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    if (pUart && pUart->hPort)
    {
        CloseHandle(pUart->hPort);
        pUart->hPort = NULL;
    }
}

int
WinUartRead(
    IN uhandle_t    hUart,
    IO void         *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puRead,   
    IN unsigned int uWaitTime   
    )
{
	int Retval;
	DWORD dwRead;
	DWORD dwTotal = 0;
    DWORD dwStart = 0;
    WinUart_T *pUart = (WinUart_T *)hUart;

    DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    dwStart = GetTickCount();

    for ( ; ; )
    {
        /* Clear the read count */
        dwRead = 0;

        /* Attempt to read from the port */
	    Retval = ReadFile(pUart->hPort, 
                          (char *)pBuff + dwTotal, 
                          uLength - dwTotal, 
                          &dwRead, 
                          NULL);

        /* If the read failed, and not a timeout assume hard fault */
        if (Retval == FALSE && GetLastError() != ERROR_TIMEOUT)
        {
            DBG_MSG(DBG_ERROR, "ReadFile failed with 0x%08x\n", GetLastError());
            Retval = E_FAIL;
            CHECK_RETVAL(Retval, ExitOnFailure);
        }

        /*  Keep track of the total number of bytes we have received */
        dwTotal = dwTotal + dwRead;

        /* Is the buffer full */
        if (dwTotal == uLength)
        {
            Retval = S_OK;
            break;
        }

        /* Have we exceeded the max wait time */
        if ((GetTickCount() - dwStart) > uWaitTime)
        {
            /* We succeed the call if data was read */
            Retval = dwTotal ? S_OK : E_TIMEOUT;
            break;
        }

        /* Yield some time */
        Sleep(1);
    }

    /* Optionally return the bytes we have read */
    if (puRead)
    {
        *puRead = dwTotal;
    }

ExitOnFailure:

    return Retval;
}

int
WinUartWrite(
    IN uhandle_t    hUart,
    IO const void   *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puWritten,   
    IN unsigned int uWaitTime   
    )
{
    int Retval;
    DWORD dwWritten;
	DWORD dwTotal = 0;
    DWORD dwStart = 0;
    WinUart_T *pUart = (WinUart_T *)hUart;

    DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    for ( ; ; )
    {
        /* Clear the write count */
        dwWritten = 0;

        /* write the data to the uart */
	    Retval = WriteFile(pUart->hPort, 
                           (char *)pBuff + dwTotal, 
                           uLength - dwTotal, 
                           &dwWritten, 
                           NULL);

        /* If the read failed, and not a timeout assume hard fault */
        if (Retval == FALSE && GetLastError() != ERROR_TIMEOUT)
        {
            DBG_MSG(DBG_ERROR, "WriteFile failed with 0x%08x\n", GetLastError());
            Retval = E_FAIL;
            break;
        }

        /* Update our the running total of bytes written */
        dwTotal = dwTotal + dwWritten;

        /* Has all of the data been written */
        if (dwTotal == uLength)
        {
            Retval = S_OK;
            break;
        }

        /* Have we exceeded the max wait time */
        if ((GetTickCount() - dwStart) > uWaitTime)
        {
            /* We succeed the call if any data was written */
            Retval = dwTotal ? S_OK : E_TIMEOUT;
            break;
        }

        /* Yield some time */
        Sleep(1);
    }

    if (puWritten)
    {
	    *puWritten = dwWritten;
    }

    return Retval;
}

int
WinUartSetStatus(
    IN uhandle_t    hUart,
    IN unsigned int uState
    )
{
    int Retval;
    WinUart_T *pUart = (WinUart_T *)hUart;
    DWORD dwModemStatus;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = WinUartLookup(uState, 
                           gModemStatusTable, 
                           COUNTOF(gModemStatusTable), 
                           (unsigned int *)&dwModemStatus);
    CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = EscapeCommFunction(pUart->hPort, dwModemStatus);
    Retval = Retval ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

ExitOnFailure:

    return Retval;
}

int
WinUartGetStatus(
    IN uhandle_t    hUart,
    OUT unsigned int *puState
    )
{
    int Retval;
    WinUart_T *pUart = (WinUart_T *)hUart;
    DWORD dwModemStatus = 0;

	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    Retval = GetCommModemStatus(pUart->hPort, &dwModemStatus);
    Retval = Retval ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

    *puState = 0;

    if (dwModemStatus & MS_CTS_ON)
    {
        *puState = *puState | UART_STATUS_CTS;
    }

    if (dwModemStatus & MS_DSR_ON)
    {
        *puState = *puState | UART_STATUS_DSR;
    }

    if (dwModemStatus & MS_RING_ON)
    {
        *puState = *puState | UART_STATUS_RI;
    }

    if (dwModemStatus & MS_RLSD_ON)
    {
        *puState = *puState | UART_STATUS_DCD;
    }

ExitOnFailure:

    return Retval;
}

/*** Private Functions ******************************************************/

int
WinUartLookup(
    unsigned int        uNormal,
    const WinEntry_T    *pEntry,
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
WinUartConvertSettings(
    unsigned int *puRate, 
    unsigned int *puBits, 
    unsigned int *puParity, 
    unsigned int *puStop
    )
{
    int Retval;

	DBG_MSG(DBG_NONE, "%s\n", __FUNCTION__);

    Retval = WinUartLookup(*puRate, gBaudTable, COUNTOF(gBaudTable), puRate);
    CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = WinUartLookup(*puBits, gDataBitsTable, COUNTOF(gDataBitsTable), puBits);
    CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = WinUartLookup(*puParity, gParityTable, COUNTOF(gParityTable), puParity);
    CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = WinUartLookup(*puStop, gStopBitsTable, COUNTOF(gStopBitsTable), puStop);
    CHECK_RETVAL(Retval, ExitOnFailure);

ExitOnFailure:

    return Retval;
}

