#include "uart.h"

void (*receChar)(u8 ch) = NULL;


static SerialBuffType m_SendBuff = SerialBuffDefault;

static SerialBuffType *sb = &m_SendBuff;

void BspUsart1Init(void)
{
    GPIO_InitTypeDef GpioInitdef;
    USART_InitTypeDef UsartInitdef;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);  

    GpioInitdef.GPIO_Pin = GPIO_Pin_9; //
    GpioInitdef.GPIO_Speed = GPIO_Speed_10MHz;
    GpioInitdef.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GpioInitdef);

    GpioInitdef.GPIO_Pin = GPIO_Pin_10; //
    GpioInitdef.GPIO_Speed = GPIO_Speed_10MHz;
    GpioInitdef.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GpioInitdef);    

    USART_DeInit(USART1);

    USART_StructInit(&UsartInitdef);
    UsartInitdef.USART_BaudRate = 115200;
    UsartInitdef.USART_StopBits = USART_StopBits_1;
    UsartInitdef.USART_WordLength = USART_WordLength_8b;
    UsartInitdef.USART_Parity = USART_Parity_No;

    UsartInitdef.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &UsartInitdef);

    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART1, ENABLE);
}

void BspUsart1Close(void)
{
    while (sb->eTXIdle != TRUE);
    USART_Cmd(USART1, DISABLE);

    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    USART_ITConfig(USART1, USART_IT_TC, DISABLE);

    USART_ClearITPendingBit(USART1, USART_IT_TXE);
    USART_ClearITPendingBit(USART1, USART_IT_TC);
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);	
}

u16 BspUsart1Send(u8 *buf, u16 len)
{
    if( (0 == sb->len) && (len > 0) && (len <= USART1_BUFF_LANGTH) && (sb->eTXIdle == TRUE))
    {
        USART_SendData(USART1, *buf++);
        sb->eTXIdle = FALSE;
        sb->len = len-1;
        sb->ind = 0;
        memcpy(sb->buf, buf, len - 1);
        USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
        return TRUE;
    }

    return FALSE;
}

void BspUsart1IRQCallBack(void *fun)
{
    receChar = (void (*)(u8))fun;
}

void USART1_IRQHandler(void)
{
    if(SET == USART_GetITStatus(USART1, USART_IT_TXE))
    {
        USART_ClearITPendingBit(USART1, USART_IT_TXE);
        if (sb->len > 0 )
        {
            USART_SendData(USART1, sb->buf[sb->ind++]);
            sb->len--;
        }
        else
        {
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
            USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        }
    }
    else if (USART_GetITStatus(USART1, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        USART_ITConfig(USART1, USART_IT_TC, DISABLE);
        sb->len = 0;
        sb->eTXIdle = TRUE;
    }
    else if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        u8 ch;
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);	
        ch = USART_ReceiveData(USART1);

        if (receChar != NULL)
        {
            (*receChar)(ch);
            BspTim3Open();      
        }
    }	
}
