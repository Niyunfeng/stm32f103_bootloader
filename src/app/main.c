#include "pub.h"
#include "led.h"
#include "uart.h"
#include "delay.h"

void BSP_Init(void);

u8 data[] = "0123456789\r\n";

int main(void)
{		
    BSP_Init();
    delay_init();
    USART_Configuration();
    led_init();
    led_on(0);  led_on(1);
    while(1)
    {
        if(Usart.RxdState == SET)
        {
            Usart.RxdState = RESET;
            USARTx_SendBuf(Usart.RxdBuf,Usart.RxdIndex);
        }
    };
}













