#include "led.h"

static struct
{
	GPIO_TypeDef *port;
	uint16_t pin;
}SYS_LED[] =
{
	{GPIOB, GPIO_Pin_5},
    {GPIOE, GPIO_Pin_5},
};

void led_init(void)
{
	u8 i;
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	for(i=0; i<ARRAY_SIZE(SYS_LED); i++)
	{
		GPIO_InitStructure.GPIO_Pin = SYS_LED[i].pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

		GPIO_Init(SYS_LED[i].port, &GPIO_InitStructure);
	}
}

//根据LED电路设置引脚电平
void led_on(unsigned char n)
{
	SYS_LED[n].port->BRR = SYS_LED[n].pin;  //复位
}
void led_off(unsigned char n)
{
    SYS_LED[n].port->BSRR = SYS_LED[n].pin;	//置位
	
}


