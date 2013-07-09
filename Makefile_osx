CC=cc
CFLAGS=-Wall -g -DDEBUG -DOSX 

all:ssiprotocol.app

OBJS=debug.o\
	common.o\
	portable.o\
	uart.o\
	osxuart.o\
	ssiprotocol.o\
    main.o\

debug.o: Debug.h Debug.c
	$(CC) $(CFLAGS) -c Debug.c

common.o: Common.h Common.c
	$(CC) $(CFLAGS) -c Common.c

portable.o : Portable.h Portable.c
	$(CC) $(CFLAGS) -c Portable.c

uart.o : Uart.h Uart.c
	$(CC) $(CFLAGS) -c Uart.c

osxuart.o : OsxUart.h OsxUart.c
	$(CC) $(CFLAGS) -c OsxUart.c

ssiprotocol.o : SSIProtocol.h SSIProtocol.c
	$(CC) $(CFLAGS) -c SSIProtocol.c

main.o: Main.h Main.c
	$(CC) $(CFLAGS) -c Main.c

ssiprotocol.app: $(OBJS)
	$(CC) $(OBJS) -o ssiprotocol.app

clean:
	rm *.o *.app 
