#ifndef _DOWNLOAD_H_
#define _DOWNLOAD_H_

#include "pub.h"
#include "common.h"

void FLASH_ProgramStart(void);
void FLASH_ProgramDone(void);
u32 FLASH_WriteBank(u8 *pData, u32 addr, u16 size);

#endif


