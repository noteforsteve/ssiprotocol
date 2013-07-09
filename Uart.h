#ifndef UART_H
#define UART_H

#define UART_TESTS

#define UART_RATE_9600      9600
#define UART_RATE_14400     14400
#define UART_RATE_19200     19200
#define UART_RATE_38400     38400
#define UART_RATE_56000     56000
#define UART_RATE_57600     57600
#define UART_RATE_115200    115200
#define UART_RATE_128000    128000
#define UART_RATE_256000    256000

#define UART_DATA_BITS_7    7
#define UART_DATA_BITS_8    8

#define UART_PARITY_NONE    0
#define UART_PARITY_EVEN    1
#define UART_PARITY_ODD     2
#define UART_PARITY_MARK    3
#define UART_PARITY_SPACE   4

#define UART_STOP_1         1
#define UART_STOP_1_5       2
#define UART_STOP_2         3

#define UART_STATUS_SETRTS  0      
#define UART_STATUS_CLRRTS  1      
#define UART_STATUS_SETDTR  2      
#define UART_STATUS_CLRDTR  3      

#define UART_STATUS_CTS     1 << 4 
#define UART_STATUS_DSR     1 << 5 
#define UART_STATUS_RI      1 << 6 
#define UART_STATUS_DCD     1 << 7 
#define UART_STATUS_MASK    0x00F0

int 
UartCtor(
    OUT uhandle_t   *phUart
    );

void
UartDtor(
    IN uhandle_t   	hUart
    );

int
UartOpen(
    IN uhandle_t   	hUart,
    IN const char   *pName,
    IN unsigned int uRate,
    IN unsigned int uDataBits,
    IN unsigned int uParity,
    IN unsigned int uStopBits
    );

void
UartClose(
    IN uhandle_t   	hUart
    );

/*
 * Read uLength bytes and store in pBuff.  The routine will return 
 * as soon as uLength bytes are available or wait uWaitTime before 
 * returning.  If the function does not receive uLength bytes and 
 * waits, it will return E_TIMEOUT if zero bytes are received or it 
 * will return S_OK indicating bytes read, and *puRead will be set to 
 * number of bytes received.
 * 
 * Returns:
 * E_XXX if a failure occurrs, bad handle, invalid port, port unplugged
 * E_TIMEOUT if the uWaitTime expired and no data was read 
 * S_OK data was read, use *puRead for actual byte count
 */
int
UartRead(
    IN uhandle_t   	hUart,     	/* Uart instance handle 					*/
    IO void         *pBuff,     /* Pointer where to return bytes read       */
    IN unsigned int uLength,    /* Length of the pBuff in bytes             */
    OUT unsigned int *puRead,   /* Can be NULL, return bytes read           */
    IN unsigned int uWaitTime   /* Time in milli-seconds                    */
    );

/*
 * Write uLength bytes to the specified uart.  The routine will return 
 * as soon as uLength bytes are written or wait uWaitTime before 
 * returning.  If the function does not write uLength bytes and 
 * waits, it will return E_TIMEOUT if zero bytes are written or it 
 * will return S_OK indicating some bytes were written and the caller  
 * must look at *puWritten to determine the number of bytes written.
 * 
 * Returns:
 * E_XXX if a failure occurrs, bad handle, invalid port, port unplugged
 * E_TIMEOUT if the uWaitTime expired and no data was written 
 * S_OK data was written, use *puWritten for actual byte count
 */
int
UartWrite(
    IN uhandle_t   	hUart,         	/* Uart instance handle 					*/
    OUT const void  *pBuff,         /* Pointer where to return bytes read       */
    IN unsigned int uLength,        /* Length of the pBuff in bytes             */
    OUT unsigned int *puWritten,    /* Can be NULL, return bytes written        */
    IN unsigned int uWaitTime       /* Time in milli-seconds                    */
    );

/* 
 * Set the state of the modem status signals
 */
int
UartSetStatus(
    IN uhandle_t   	hUart,         	/* Uart instance handle 					*/
    IN unsigned int uState			/* New modem status to set 					*/
    );

/*
 * Get the state of the modem status signals
 */
int
UartGetStatus(
    IN uhandle_t   	hUart,         	/* Uart instance handle 					*/
    OUT unsigned int *puState       /* Current state of the modems signals 		*/
    );

#if defined (UART_TESTS)

int
UartTest(
	IN const char 	*pszPort
	);

int
UartTest1(
	IN const char 	*pszPort
	);

#endif

#endif

