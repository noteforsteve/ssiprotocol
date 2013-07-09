#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Portable.h"
#include "Common.h"
#include "Uart.h"
#include "SSIProtocol.h"
#include "Main.h"

#define DEBUG_MODULE
//#define DEBUG_LEVEL DBG_TRACE|DBG_WARN|DBG_ERROR 
#define DEBUG_LEVEL DBG_WARN|DBG_ERROR 
#include "Debug.h"

// The device used for this demo asserts the DCD signal when connected 
int 
WaitForDevice(
    IN uhandle_t    hUart,
    IN unsigned int Wait
    )
{
    int Retval;
    unsigned int Start;
    unsigned int Status;

    // Adjust to seconds
    Wait = Wait * 1000;

    Start = PortableGetTick();

    for ( ; ; )
    {
        Retval = UartGetStatus(hUart, &Status);
    	CHECK_RETVAL(Retval, ExitOnFailure);

        if (Status & UART_STATUS_DCD)
        {
             break;
        }

        if (PortableGetTick() - Start > Wait)
        {
            Retval = E_TIMEOUT;
            break;
        }

        PortableSleep(250);
    }

ExitOnFailure:

    return Retval;
}

int 
main(
	int 	ac, 
	char 	**av
	)
{
	int Retval = 0;
	uhandle_t hUart = 0;
    uhandle_t hProtocol = 0;
    SSIPacket_T Packet;
    
	DBG_MSG(DBG_TRACE, "%s\n", __FUNCTION__);

    printf("usage: ssiprotocol <port> <...>\n");

	Retval = UartCtor(&hUart);
	CHECK_RETVAL(Retval, ExitOnFailure);

	Retval = UartOpen(hUart, 
    				  av[1],
					  UART_RATE_57600,
					  UART_DATA_BITS_8,
					  UART_PARITY_NONE,
					  UART_STOP_1);
	CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = SSIProtocolCtor(hUart, &hProtocol);
	CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = WaitForDevice(hUart, 60);
	CHECK_RETVAL(Retval, ExitOnFailure);

    // Request the engine version 
    Retval = SSIPacketMake(0, 
                           SSI_OPCODE_REQUEST_REVISION,
                           SSI_SOURCE_HOST,
                           SSI_STATUS_NO_RETRANSMIT,
                           NULL,
                           0,
                           &Packet);
	CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = SSIProtocolSend(hProtocol, &Packet, 5000);
	CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = SSIProtocolRecv(hProtocol, &Packet, 5000);
	CHECK_RETVAL(Retval, ExitOnFailure);

    Retval = Packet.Header.OpCode == SSI_OPCODE_REPLY_REVISION ? S_OK : E_FAIL;
	CHECK_RETVAL(Retval, ExitOnFailure);

    Packet.Payload.Data[Packet.Header.Length-sizeof(Packet.Header)] = 0; 
    printf("Engine Revison: %s\n", Packet.Payload.Data);

    for ( ; ; )
    {
        // Trigger the scan engine and wait for the ack response
        Retval = SSIPacketMake(0, 
                               SSI_OPCODE_START_DECODE,
                               SSI_SOURCE_HOST,
                               SSI_STATUS_NO_RETRANSMIT,
                               NULL,
                               0,
                               &Packet);
	    CHECK_RETVAL(Retval, ExitOnFailure);

        Retval = SSIProtocolSend(hProtocol, &Packet, 5000);
	    CHECK_RETVAL(Retval, ExitOnFailure);

        Retval = SSIProtocolRecv(hProtocol, &Packet, 5000);
	    CHECK_RETVAL(Retval, ExitOnFailure);

        Retval = Packet.Header.OpCode == SSI_OPCODE_ACK ? S_OK : E_FAIL;
	    CHECK_RETVAL(Retval, ExitOnFailure);

        Retval = SSIProtocolRecv(hProtocol, &Packet, 3000);
        if (Retval == S_OK)
        {
			if (Packet.Header.OpCode == SSI_OPCODE_DECODE_DATA)
			{
            	// Assume the bar code is asii, and zero terminate
            	Packet.Payload.Data[Packet.Header.Length-sizeof(Packet.Header)] = 0; 
            	printf("Symbology Id: %d\n", Packet.Payload.Data[0]);
            	printf("Decode Data: %s\n", &Packet.Payload.Data[1]);
            	PortableSleep(1000);
			}
			else
			{
            	SSIPacketDump(&Packet);
			}
        }
        else
        {
            // Trigger the scan engine and wait for the ack response
            Retval = SSIPacketMake(0, 
                                   SSI_OPCODE_STOP_DECODE,
                                   SSI_SOURCE_HOST,
                                   SSI_STATUS_NO_RETRANSMIT,
                                   NULL,
                                   0,
                                   &Packet);
	        CHECK_RETVAL(Retval, ExitOnFailure);

            Retval = SSIProtocolSend(hProtocol, &Packet, 5000);
	        CHECK_RETVAL(Retval, ExitOnFailure);

	        Retval = SSIProtocolRecv(hProtocol, &Packet, 5000);
		    CHECK_RETVAL(Retval, ExitOnFailure);
        }
    }

ExitOnFailure:

    /* Bad things happen if we abruptly close the bluetooth port */
    PortableSleep(5*1000);

    SSIProtocolDtor(hProtocol);

	UartClose(hUart);

	UartDtor(hUart);

	return Retval;
}

    
