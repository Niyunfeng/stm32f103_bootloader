#include "BspTime3.h"

static void (*IRQHandler)(void) = NULL;

void BspTim3Init(void)
{
    TIM_TimeBaseInitTypeDef timbase;	
    u16 cnt = 170 * 72;     //170us 触发

    TIM_DeInit(TIM3); 
    timbase.TIM_CounterMode = TIM_CounterMode_Up;
    timbase.TIM_ClockDivision = TIM_CKD_DIV1;
    timbase.TIM_Period = cnt - 1;
    timbase.TIM_Prescaler = 0; 	        // 1分频
    TIM_TimeBaseInit(TIM3, &timbase);

    TIM_SetCounter(TIM3, 0);
    TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
    TIM_ClearITPendingBit( TIM3, TIM_IT_Update);

    /* TIMX enable counter */
    TIM_Cmd(TIM3, DISABLE);
}

void BspTim3Open(void)
{
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}

void BspTim3Close(void)
{
    TIM_Cmd(TIM3, DISABLE);
    TIM_SetCounter(TIM3, 0);
    TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
}

void BspTim3SetIRQCallBack(void *fun)
{
    IRQHandler = (void (*)(void))fun;
}


void TIM3_IRQHandler(void)
{
    if( SET == TIM_GetITStatus(TIM3,TIM_IT_Update) )
    {
        TIM_ClearITPendingBit( TIM3, TIM_IT_Update);

        if (IRQHandler != NULL)
        {
            (*IRQHandler)();			//中断函数
            BspTim3Close();
        }
    }
}

