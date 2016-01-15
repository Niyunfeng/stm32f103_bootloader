#ifndef _DrvUsart1_h_
#define _DrvUsart1_h_

#include "pub.h"
#include <string.h>
#include "BspTime3.h"

#define USART1_BUFF_LANGTH     1048


typedef struct {
    bool volatile eTXIdle;    
    bool volatile eRXIdle;    
	u32 len;
	u16 ind;	
	u8  buf[USART1_BUFF_LANGTH];
}SerialBuffType;		//·¢ËÍÔÝ´æÇø

#define SerialBuffDefault {FALSE,FALSE,0,0,{0,}}


void BspUsart1Init(void);
void BspUsart1Close(void);

u16 BspUsart1Send(u8 *buf, u16 len);
u16 BspUsart1Receive(u8 *buf);

u8 Usart1ReceiveByte(void);
void BspUsart1IRQCallBack(void *fun);


#endif
/********************** END ***************************************************/


