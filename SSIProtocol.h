#ifndef SSIPROTOCOL_H
#define SSIPROTOCOL_H

typedef enum
{
	SSI_MIN_PACKET                  = 4,
	SSI_MAX_PAYLOAD                 = 253,
	SSI_MAX_PACKET                  = 257,

    SSI_SOURCE_HOST			        = 0x04,
    SSI_SOURCE_DEVICE				= 0x00,

    SSI_STATUS_NO_RETRANSMIT		= 0x00,
    SSI_STATUS_RETRANSMIT			= 0x01,
    SSI_STATUS_TEMPORARY			= 0x00,
    SSI_STATUS_PERMANENT			= 0x08,
	
	SSI_OPCODE_ACK	                = 0xD0,
	SSI_OPCODE_NAK	                = 0xD1,

    SSI_OPCODE_PARAM_SEND		    = 0xC6,
    SSI_OPCODE_PARAM_REQUEST	    = 0xC7,
    SSI_OPCODE_PARAM_DEFAULTS	    = 0xC8,
    SSI_OPCODE_START_DECODE         = 0xE4,
    SSI_OPCODE_STOP_DECODE	        = 0xE5,
    SSI_OPCODE_DECODE_DATA	        = 0xF3,
    SSI_OPCODE_SLEEP	            = 0xEB,

    SSI_OPCODE_REQUEST_REVISION	    = 0xA3,
    SSI_OPCODE_REPLY_REVISION       = 0xA4,

} SSIConstants;

typedef struct
{
    uint8_t Length;
    uint8_t OpCode;
    uint8_t Source;
    uint8_t Status;
} SSIHeader_T;

typedef struct
{
	uint8_t Data[SSI_MAX_PAYLOAD];
} SSIPayload_T;

typedef struct
{
	uint8_t High;
	uint8_t Low;
} SSICheckSum_T;

typedef struct
{
    SSIHeader_T     Header;
    SSIPayload_T    Payload;
    SSICheckSum_T   CheckSum;
    uint8_t         PayloadLen;
} SSIPacket_T;

int
SSIPacketMake(
    IN uint8_t          Length,
    IN uint8_t          OpCode,
    IN uint8_t          Source,
    IN uint8_t          Status,
    IN const uint8_t    *pPayload,
    IN uint8_t          PayloadLen,
    IO SSIPacket_T      *pPacket
    );

void
SSIPacketDump(
    IN const SSIPacket_T *pPacket
    );

/*** SSIProtocol *************************************************************/

int
SSIProtocolCtor(
    IN uhandle_t        hUart,
    IO uhandle_t        *phProtocol
    );

void
SSIProtocolDtor(
    IN uhandle_t        hProtocol
    );

int
SSIProtocolSend(
    IN uhandle_t        hProtocol,
    IN SSIPacket_T      *pPacket,    
    IN uint32_t         uMaxWait
    );

int
SSIProtocolRecv(
    IN uhandle_t        hProtocol,
    IN SSIPacket_T      *pPacket,
    IN uint32_t         uMaxWait
    );

#endif
