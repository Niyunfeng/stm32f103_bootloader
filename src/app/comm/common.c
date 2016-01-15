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
static vu32 m_ExtFlashCounter = 0;       //�ⲿFLASH������������

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
    BspTim3Open();      //��ʱ�����¼���
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

            if (flg && IS_TIMEOUT_1MS(eTim1, 100))  //����ȷ��
                *peStat = eCOMDisplay;
        }
        *pLen = 0;
    }
    
    if (IS_TIMEOUT_1MS(eTim2, 200))
    {
        JumpToApp(); 
        Print("\n\r����ʧ��!");
        while (1);
     }
}

static void DisplayMessage(u8 *pData, u32 *pLen, volatile eCOM_STATUS *peStat)
{
    *pLen = 1;
    strcpy((char *)pData, "\r*********************************************************\r\n");
    strcat((char *)pData, "1.����Ӧ��������\r\n");  
    strcat((char *)pData, "2.����APP����\r\n");
    strcat((char *)pData, "*********************************************************\r\n");
    strcat((char *)pData, "��ѡ��:");
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
            Print("1\r\n��ѡ��Ҫ�����ļ�");
            break;    
            
        case '2': 
            *peStat = eCOMChoose; 
            Print("2\r\n���г���...");
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
        if (FunReceEnter) (*FunReceEnter)();    //��ʼ����
            break;
            
        case YM_FILE_DATA: 
            if (FunWrite) (*FunWrite)(pArray, m_ProgramAddr, len);  //�������ݺ���
            m_ProgramAddr += len;
            break;
        case YM_EXIT: 
            if (FunReceExit) (*FunReceExit)();      //������Ϻ���
            
            FunReceEnter = NULL;
            FunWrite = NULL;
            FunReceExit = NULL;
            Print("\r\n���г���...");
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
    case eCOMChoose:    //�жϽ��� IAP���� ����APP����
        FunCurrentProcess = AppChoose;
        break;
        
    case eCOMDisplay:   //IAP������ʾ
        FunCurrentProcess = DisplayMessage;
        break;
        
    case eCOMInput:     //IAP����ѡ��
        FunCurrentProcess = InputSelect;
        break;

    case eCOMReceive:   //YMODEM ��������
        FunCurrentProcess = ReceiveData;
        break;
        
    default:
        m_Mode = eCOMChoose;
        break;
    }
    (*FunCurrentProcess)((u8 *)(m_ReceData.buf), (u32 *)&(m_ReceData.len), &m_Mode);
}
