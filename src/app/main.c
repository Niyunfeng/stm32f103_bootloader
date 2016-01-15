#include "pub.h"
#include "common.h"
#include "led.h"

void BspInit(void);

int main(void)
{		
    u8 flg = 0;
    
    BspInit();
    CommonInit();

    while(1)
    {
        CommonExec();
        if (IS_TIMEOUT_1MS(eTim1, 500))
        {
            flg? led_on(0): led_off(0);
            flg = !flg;
        }
    };
}








