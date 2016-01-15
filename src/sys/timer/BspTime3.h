#ifndef _DrvTime3_h_
#define _DrvTime3_h_

#include "pub.h"

void BspTim3Init(void);
void BspTim3Open(void);
void BspTim3Close(void);
void BspTim3SetIRQCallBack(void *fun);

#endif


