
#include "Terminal.h"
#include "../System.h"
#include "../LEDHook.h"

// ptc is a layer to handle uart output
void ptc(char c){
	do{
			/* Wait until it is possible to put data into TX data register.*/
	} while(!( (USARTD1.STATUS & USART_DREIF_bm) != 0));
	USARTD1.DATA = c;
}
// pts is a loop of pts
void pts(const char * str, uint16_t len){
	uint8_t i = 0;
	if (len){
		for(; i < len; i++){
			ptc(str[i]);
		}
	} else {
		while (str[i] != 0){
			ptc(str[i++]);
		} ptc(0);
	}
}
// ftc is a layer to handle uart input
volatile int16_t gtc(){
	volatile int16_t recv;
	if (USARTD1.STATUS & USART_RXCIF_bm) {
		recv = USARTD1.DATA;
		//ptc((char)recv); // get rid of this get rid of this get rid of this
		return (int16_t) recv;
	} else {
		return -1;
	}
}

#define INIT_DELAY		(2000 / SYSTEM_TICK_MS)


uint8_t TerminalBuffer[TERMINAL_BUFFER_SIZE];
TerminalStateEnum TerminalState = TERMINAL_UNINITIALIZED;

// TerminalSendString was part of Terminmal.c for USB so I left it
void TerminalSendString(const char* s) {
	pts(s, 0);
}
// TerminalSendStringp is a '\=' send string
void TerminalSendStringP(const char* s) {
    char c;

    while( (c = pgm_read_byte(s++)) != '\0' ) {
        TerminalSendChar(c);
    }
}

/*
void TerminalSendHex(void* Buffer, uint16_t ByteCount)
{
    char* pTerminalBuffer = (char*) TerminalBuffer;

    BufferToHexString(pTerminalBuffer, sizeof(TerminalBuffer), Buffer, ByteCount);

    TerminalSendString(pTerminalBuffer);
}

*/

// A circular buffer
char bigBuffer[142];
uint16_t writeC = 0, readC = 0, mark = 0;
// returns values from bigBuffer if a \r was found
int16_t getByteFromBuffer(void){
	if (readC == 142){
		readC = 0;
	}
	if (mark){
		if ((bigBuffer[readC] == '\r')||(bigBuffer[readC] == 0x01)||(bigBuffer[readC] == 0x04)) {
			mark--;
		}
		return bigBuffer[readC++];
	}
	return -1;
}
// writes to the circular buffer
void newUART(void){
	volatile int16_t rcvLocal = gtc();
	if (rcvLocal != -1){
		bigBuffer[writeC++] = (char)rcvLocal;
		if (writeC == 142) {
			writeC = 0;
		}
		if (((char)rcvLocal == '\r') || ((char)rcvLocal == 0x01) || ((char)rcvLocal == 0x04)) {
			mark++;
		}
	}
}

void TerminalSendBlock(const void* Buffer, uint16_t ByteCount)
{
  	pts(Buffer, ByteCount); 
}


static void ProcessByte(void) {
		int16_t Byte = getByteFromBuffer();

    if (Byte >= 0) {
        /* Byte received */
    	LEDHook(LED_TERMINAL_RXTX, LED_PULSE);

        if (XModemProcessByte(Byte)) { // check xmodem
            /* XModem handled the byte */
        } else if (CommandLineProcessByte(Byte)) {
            /* CommandLine handled the byte */
        }
    }
}

static void SenseVBus(void)
{
		TerminalState = TERMINAL_INITIALIZED;
}

void TerminalInit(void)
{
   // Could put UART Init here 
}

void TerminalTask(void)
{
	if (TerminalState == TERMINAL_INITIALIZED) {

		// maybe these were used to actually send out what was in a buffer?

		ProcessByte();
	}
}

void TerminalTick(void)
{
	SenseVBus();

	if (TerminalState == TERMINAL_INITIALIZED) {
		XModemTick();
		CommandLineTick();
	}
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDHook(LED_TERMINAL_CONN, LED_ON);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDHook(LED_TERMINAL_CONN, LED_OFF);
}


/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
;
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
;
}


