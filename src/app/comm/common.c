#include "common.h"

static FunVoidType JumpToApplication;
static FunVoidType FunReceEnter = NULL;
static FunVoidType FunReceExit = NULL;
static FunWriteType FunWrite = NULL;
static FunProcessType FunCurrentProcess = NULL;

static u32 m_JumpAddress;
static u32 m_ProgramAddr = ApplicationAddress;
static volatile SerialBuffType m_ReceData = SerialBuffDefault;

static volatile eCOM_STATUS m_Mode = eCOMChoose;
static vu32 m_FlashAddress = 0;
static vu32 m_ExtFlashCounter = 0;       //外部FLASH擦除的扇区号

extern void BspClose(void);

static void Print(u8 *str)
{
    u16 len = 0;

    len = strlen((const char *)str);

    while (BspUsart1Send(str, len) != TRUE);
}

static void ReceOneChar(u8 ReceCharacter)
{
    if (m_ReceData.ind >= USART1_BUFF_LANGTH)
        return;
        
    if (m_ReceData.len > 0)
        return;
        
    m_ReceData.buf[m_ReceData.ind++] = ReceCharacter;
    BspTim3Open();      //定时器重新计数
}

static void TimEndHandle(void)
{
    BspTim3Close();

    m_ReceData.len = m_ReceData.ind;
    m_ReceData.ind = 0;
}

static void JumpToApp(void)
{
    if (((*(vu32*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
    { 
        BspClose();
        /* Jump to user application */
        m_JumpAddress = *(vu32*) (ApplicationAddress + 4);
        JumpToApplication = (FunVoidType) m_JumpAddress;

        /* Initialize user application's Stack Pointer */
        MSR_MSP(*(vu32*) ApplicationAddress);
        JumpToApplication();
    }
}

static void AppChoose(u8 *pData, u32 *pLen, volatile eCOM_STATUS *peStat)
{
    static u8 flg = 0;
    
    if (*pLen > 0)
    {
        if ((*pData == 'C') || (*pData == 'c'))
        {
            if (flg == 0)
                flg++;

            if (flg && IS_TIMEOUT_1MS(eTim1, 100))  //二次确认
                *peStat = eCOMDisplay;
        }
        *pLen = 0;
    }
    
    if (IS_TIMEOUT_1MS(eTim2, 200))
    {
        JumpToApp(); 
        Print("\n\r运行失败!");
        while (1);
     }
}

static void DisplayMessage(u8 *pData, u32 *pLen, volatile eCOM_STATUS *peStat)
{
    *pLen = 1;
    strcpy((char *)pData, "\r*********************************************************\r\n");
    strcat((char *)pData, "1.更新应用区程序；\r\n");  
    strcat((char *)pData, "2.运行APP程序。\r\n");
    strcat((char *)pData, "*********************************************************\r\n");
    strcat((char *)pData, "请选择:");
    *peStat = eCOMInput;
    
    Print(pData);
    *pLen = 0;
}

static void InputSelect(u8 *pData, u32 *pLen, volatile eCOM_STATUS *peStat)
{
    if (*pLen > 0)
    {
        switch (*pData)
        {
        case '1': 
            m_ProgramAddr = ApplicationAddress; 
            FunReceEnter = FLASH_ProgramStart;
            FunWrite = FLASH_WriteBank;
            FunReceExit = FLASH_ProgramDone;
            *peStat = eCOMReceive;
            Print("1\r\n请选择要发送文件");
            break;    
            
        case '2': 
            *peStat = eCOMChoose; 
            Print("2\r\n运行程序...");
            break;    
            
        default :break;
        }
        *pLen = 0;
    }
}

static void ReceiveData(u8 *pData, u32 *pLen, volatile eCOM_STATUS *peStat)
{
    u8 pArray[1028] = {0,};
    int len = 0;
    
    switch (YmodemReceive((char *)(pData), (int *)pLen, (char *)pArray, (int *)&len))
    {
        case YM_FILE_INFO: 
        if (FunReceEnter) (*FunReceEnter)();    //开始函数
            break;
            
        case YM_FILE_DATA: 
            if (FunWrite) (*FunWrite)(pArray, m_ProgramAddr, len);  //接收数据函数
            m_ProgramAddr += len;
            break;
        case YM_EXIT: 
            if (FunReceExit) (*FunReceExit)();      //接收完毕函数
            
            FunReceEnter = NULL;
            FunWrite = NULL;
            FunReceExit = NULL;
            Print("\r\n运行程序...");
            *peStat = eCOMChoose;
            break;
    }

}

void CommonInit(void)
{
    BspTim3SetIRQCallBack(TimEndHandle);
    BspUsart1IRQCallBack(ReceOneChar);
}

void CommonExec(void)
{
    switch (m_Mode)
    {
    case eCOMChoose:    //判断进入 IAP程序 还是APP程序
        FunCurrentProcess = AppChoose;
        break;
        
    case eCOMDisplay:   //IAP操作显示
        FunCurrentProcess = DisplayMessage;
        break;
        
    case eCOMInput:     //IAP操作选择
        FunCurrentProcess = InputSelect;
        break;

    case eCOMReceive:   //YMODEM 接收数据
        FunCurrentProcess = ReceiveData;
        break;
        
    default:
        m_Mode = eCOMChoose;
        break;
    }
    (*FunCurrentProcess)((u8 *)(m_ReceData.buf), (u32 *)&(m_ReceData.len), &m_Mode);
}
