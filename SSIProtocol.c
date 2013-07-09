#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "Common.h"
#include "Uart.h"
#include "Portable.h"
#include "SSIProtocol.h"

#define DEBUG_MODULE
//#define DEBUG_LEVEL DBG_TRACE|DBG_WARN|DBG_ERROR 
#define DEBUG_LEVEL DBG_WARN|DBG_ERROR 
#include "Debug.h"

/*** SSI Structure Defs ******************************************************/

typedef struct 
{
    uhandle_t hUart;
} SSIProtocol_T;

/*** SSI Packet Function Prototypes *******************************************/

void
SSIPacketCheckSumBlock(
	IN const void   *pData,
	IN uint8_t      uLength,
	OUT uint8_t     *puHigh,
	OUT uint8_t     *puLow
	);

int
SSIPacketCheckSumCheck(
    IN SSIPacket_T  *pPacket
	);

/*** SSI Packet Public *******************************************************/

int
SSIPacketMake(
    IN uint8_t          Length,
    IN uint8_t          OpCode,
    IN uint8_t          Source,
    IN uint8_t          Status,
    IN const uint8_t    *pPayload,
    IN uint8_t          PayloadLen,
    IO SSIPacket_T      *pPacket
    )
{
    int Retval = S_OK;

    Retval = pPacket ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

    /* If payload length is non zero, payload buffer pointer must be valid */
    if (PayloadLen)
    {
        Retval = pPayload ? S_OK : E_INVALIDARG;
        CHECK_RETVAL(Retval, ExitOnFailure);
    }

    /* If the length is zero, then caluculate it */
    if (Length == 0)
    {
        pPacket->Header.Length = sizeof(SSIHeader_T) + PayloadLen;
    }
    else
    {
        pPacket->Header.Length = Length;
    }

    pPacket->Header.OpCode = OpCode;
    pPacket->Header.Source = Source;
    pPacket->Header.Status = Status;

    if (pPacket->Payload.Data && PayloadLen && pPayload != pPacket->Payload.Data)
    {
        memcpy(pPacket->Payload.Data, pPayload, PayloadLen);
    }

    SSIPacketCheckSumBlock(pPacket, 
                           pPacket->Header.Length,
                           (uint8_t *)pPacket + pPacket->Header.Length + 0,
                           (uint8_t *)pPacket + pPacket->Header.Length + 1);

ExitOnFailure:

    return Retval;
}

/*** SSI Packet Private ******************************************************/

void
SSIPacketCheckSumBlock(
	IN const void   *pData,
	IN uint8_t      uLength,
	OUT uint8_t     *puHigh,
	OUT uint8_t     *puLow
	)
{
	uint16_t uCheckSum;
    const uint8_t *p;

    p = pData;
    uCheckSum = 0;

	for ( ; uLength; uLength = uLength - 1)
	{
		uCheckSum = uCheckSum - *p;
        p = p + 1;
	}

	*puHigh = ((uCheckSum >> 8) & 0x00FF);
	*puLow = ((uCheckSum >> 0) & 0x00FF);
}

int
SSIPacketCheckSumCheck(
    IN SSIPacket_T  *pPacket
	)
{
    int Retval;
    uint8_t uHigh;
    uint8_t uLow;

    SSIPacketCheckSumBlock(pPacket, 
                           pPacket->Header.Length,
                           &uHigh,
                           &uLow);

    if ((uHigh == *((uint8_t *)pPacket + pPacket->Header.Length + 0)) &&
        (uLow == *((uint8_t *)pPacket + pPacket->Header.Length + 1)))
    {
        Retval = S_OK;
    }
    else
    {
        Retval = E_FAIL;
    }

    return Retval;
}

void
SSIPacketDump(
    IN const SSIPacket_T *pPacket
    )
{
    DBG_DUMP(DBG_WARN, 0, pPacket, pPacket->Header.Length+2);
}

/*** SSI Protocol Public *****************************************************/

int
SSIProtocolCtor(
    IN uhandle_t        hUart,
    IO uhandle_t        *phProtocol
    )
{
    int Retval;
    SSIProtocol_T *pProtocol;

    Retval = hUart && phProtocol ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

    pProtocol = malloc(sizeof(SSIProtocol_T));

    pProtocol->hUart = hUart;

    *phProtocol = (uhandle_t)pProtocol;

ExitOnFailure:

    return Retval;
}

void
SSIProtocolDtor(
    IN uhandle_t        hProtocol
    )
{
    SSIProtocol_T *pProtocol = (SSIProtocol_T *)hProtocol;

    if (pProtocol)
    {
        pProtocol->hUart = 0;
    }
}

int
SSIProtocolSend(
    IN uhandle_t        hProtocol,
    IN SSIPacket_T      *pPacket,   
    IN uint32_t         uMaxWait
    )
{
    int Retval;
    unsigned int uWritten;
    unsigned int uWriteLen;
    SSIProtocol_T *pProtocol = (SSIProtocol_T *)hProtocol;

    Retval = (pProtocol && pPacket) ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

    uWriteLen = pPacket->Header.Length + sizeof(SSICheckSum_T);

    Retval = UartWrite(pProtocol->hUart, 
                       pPacket, 
                       uWriteLen, 
                       &uWritten, 
                       uMaxWait);
    CHECK_RETVAL(Retval, ExitOnFailure);

	DBG_MSG(DBG_TRACE, "uWritten %d\n", uWritten);

    Retval = uWritten == uWriteLen ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

ExitOnFailure:

    return Retval;
}

int
SSIProtocolRecv(
    IN uhandle_t        hProtocol,
    IN SSIPacket_T      *pPacket,
    IN uint32_t         uMaxWait
    )
{
    int Retval;
    unsigned int uReadLen;
    unsigned int uTick;
    unsigned int uTemp;
    SSIProtocol_T *pProtocol = (SSIProtocol_T *)hProtocol;

    Retval = (pProtocol && pPacket) ? S_OK : E_INVALIDARG;
    CHECK_RETVAL(Retval, ExitOnFailure);

    uTick = PortableGetTick();

	/* Read the length byte of the SSI packet */
	Retval = UartRead(pProtocol->hUart, 
                      &pPacket->Header.Length,
                      sizeof(pPacket->Header.Length),
                      &uReadLen,
                      uMaxWait);
    CHECK_RETVAL_SAFE(Retval, E_TIMEOUT, ExitOnFailure);

    Retval = uReadLen == sizeof(pPacket->Header.Length) ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

    uTick = PortableGetTick() - uTick;

    Retval = uTick < uMaxWait ? S_OK : E_TIMEOUT;
    CHECK_RETVAL(Retval, ExitOnFailure);
 
	/* Calculate the remaining wait time */
    uMaxWait = uMaxWait - uTick;

	/* Calculate the remaining byte count */
    uTemp = pPacket->Header.Length + 
			sizeof(SSICheckSum_T) - 
			sizeof(pPacket->Header.Length);

	/* Read the remaining bytes of the SSI packet */
	Retval = UartRead(pProtocol->hUart, 
                      &pPacket->Header.OpCode, 
                      uTemp,
                      &uReadLen, 
                      uMaxWait);
    CHECK_RETVAL(Retval, ExitOnFailure);

	/* Did we read what we expected */
    Retval = uReadLen == uTemp ? S_OK : E_FAIL;
    CHECK_RETVAL(Retval, ExitOnFailure);

	/* Did the check sum match */
    Retval = SSIPacketCheckSumCheck(pPacket);
    CHECK_RETVAL(Retval, ExitOnFailure);

	DBG_MSG(DBG_TRACE, "uRead %d\n", pPacket->Header.Length);

ExitOnFailure:
    
    return Retval;
}


