#ifndef OSXUART_H
#define OSXUART_H

int
OsxUartCtor(
    OUT uhandle_t   *ph
    );

void
OsxUartDtor(
    IN uhandle_t 	h       
    );

int
OsxUartOpen(
    IN uhandle_t	h,
    IN const char   *pName,
    IN unsigned int uRate,
    IN unsigned int uDataBits,
    IN unsigned int uParity,
    IN unsigned int uStopBits
    );

void
OsxUartClose(
	IN uhandle_t	h
    );

int
OsxUartRead(
	IN uhandle_t	h,
    IO void         *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puRead,   
    IN unsigned int uWaitTime   
    );

int
OsxUartWrite(
	IN uhandle_t	h,
    IO const void   *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puWritten,   
    IN unsigned int uWaitTime   
    );

int
OsxUartSetStatus(
	IN uhandle_t	h,
    IN unsigned int uState
    );

int
OsxUartGetStatus(
	IN uhandle_t	h,
    OUT unsigned int *puState
    );

#endif

