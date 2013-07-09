#ifndef WINUART_H
#define WINUART_H

int
WinUartCtor(
    OUT uhandle_t	*ph
    );

void
WinUartDtor(
    IN uhandle_t  	h
    );

int
WinUartOpen(
    IN uhandle_t  	h,
    IN const char   *pName,
    IN unsigned int uRate,
    IN unsigned int uDataBits,
    IN unsigned int uParity,
    IN unsigned int uStopBits
    );

void
WinUartClose(
    IN uhandle_t  	h
    );
    
int
WinUartRead(
    IN uhandle_t  	h,
    IO void         *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puRead,   
    IN unsigned int uWaitTime   
    );

int
WinUartWrite(
    IN uhandle_t  	h,
    IO const void   *pBuff,     
    IN unsigned int uLength,    
    OUT unsigned int *puWritten,   
    IN unsigned int uWaitTime   
    );

int
WinUartSetStatus(
    IN uhandle_t  	h,
    IN unsigned int uState
    );

int
WinUartGetStatus(
    IN uhandle_t  	h,
    OUT unsigned int *puState
    );

#endif

