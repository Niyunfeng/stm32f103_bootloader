#ifndef _UART_H_
#define _UART_H_

#include "pub.h"

//�������ã�3������ͬʱֻ֧��һ��

#define TX_LENGTH      128  //���������С
#define RX_LENGTH      1280 //���������С

typedef struct
{
    u8 TxdBuf[TX_LENGTH];  //���ڷ��ͻ���
    u8 RxdBuf[RX_LENGTH];  //���ڽ��ջ���
    u8 RxdIndex;           //���ڽ������ݸ���
    FlagStatus RxdState;           //���ڽ���״̬
}_DEF_Uart;

extern _DEF_Uart Usart;

#define UART_PORT       1       //���崮�ں�

#define UART_BAUDRATE  115200   //���崮�ڲ�����

#if UART_PORT == 1
    #define UARTX 				    USART1
    #define UARTX_IRQ 			    USART1_IRQn
    #define UARTX_DMA_IRQ           DMA1_Channel4_IRQn
    #define UARTX_DMA_Channel       DMA1_Channel4
#elif UART_PORT == 2
    #define UARTX 				    USART2
    #define UARTX_IRQ 			    USART2_IRQn
    #define UARTX_DMA_IRQ           DMA1_Channel7_IRQn
    #define UARTX_DMA_Channel       DMA1_Channel7 
#elif UART_PORT == 3
    #define UARTX 				    USART3
    #define UARTX_IRQ 			    USART3_IRQn
    #define UARTX_DMA_IRQ           DMA1_Channel2_IRQn
    #define UARTX_DMA_Channel       DMA1_Channel2    
#endif

void USART_Configuration(void);
void USARTx_SendBuf(u8* data,uint16_t len);

#endif

