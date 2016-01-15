#ifndef _DrvTime2_h_
#define _DrvTime2_h_

#include "pub.h"

enum {
    eTim1,
    eTim2,
    eTimYModem,
    eTimMax,
};

#define IS_TIMEOUT_1MS(index, count)    ((g_Tim2Array[(u16)(index)] >= (count))?  \
                                        ((g_Tim2Array[(u16)(index)] = 0) == 0): 0)


extern volatile int g_Tim2Array[(u16)eTimMax];

void BspTim2Init(void);
void BspTim2Close(void);

#endif

