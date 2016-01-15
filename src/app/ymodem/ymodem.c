#include "YModem.h"

#define SendString(a, b)    BspUsart1Send(a, b)     //���ڷ��ͺ��� a:�ַ��� b:����

static unsigned short YModemCrc(char *pData, unsigned short sLen)
{  
   unsigned short CRC_DATA = 0;    //
   unsigned short i = 0;
   
   while (sLen--)  //len����Ҫ����ĳ���
   {
       CRC_DATA = CRC_DATA^(int)(*pData++) << 8; //    
       for (i=8; i!=0; i--) 
       {
           if (CRC_DATA & 0x8000)   
               CRC_DATA = CRC_DATA << 1 ^ 0x1021;    
           else
               CRC_DATA = CRC_DATA << 1;
       }    
   }
   return CRC_DATA;
}

static int ReceivePacket (char *pRece, int sReceLen, char *pData, int *sLen, char cNum)
{
    int len = 0;            //���ݵĳ���
    int tmp = 0;

    if (pRece[0] == SOH)
    {
        len = PACKET_SIZE;
    } 
    else if (pRece[0] == STX)
    {
        len = PACKET_1K_SIZE;
    }
    else if (pRece[0] == EOT)   
    {
        return 2;               //���յ�������־
    }
    else if (pRece[0] == ETX)   
    {
        return 0;               //���յ���ֹ��־ ctrl + c
    }
    else if (pRece[0] == CA)    //���յ���ֹ��־
    {
        if (pRece[1] == CA)     //ȷ����ֹ��־
            return 0; 
        else
            return -6;          //���յ���ֹ�����־
    }
    else
    {
        return -1;              //����ͷ����
    }

    if (pRece[1] != cNum)
        return -2;              //���ݱ�Ŵ���
     
    if (pRece[2] != (char)(~cNum))
        return -3;              //���ݱ�Ų������

    if (sReceLen != (len + PACKET_OVERHEAD))
        return -4;              //�����ܳ��ȴ���

    // YModemֻЧ�����ݲ���
    tmp = YModemCrc(pRece + PACKET_HEADER, len);
    if (((pRece[sReceLen - 2] << 8) |pRece[sReceLen - 1]) != tmp)
        return -5;              //crcЧ�����

    memcpy(pData, &pRece[PACKET_HEADER], len);  //��������
    *sLen = len;

    return 1;
}


void YmodemSendChar(unsigned char ch, eYM_STAT *stat, int *errCount)
{
    while(TRUE != SendString(&ch, 1));
 
    if (ch == ACK)                      //�������������
    {
        *errCount = 0;
    }
    else if (ch == NAK)                 //����������ۼ�
    {
        (*errCount)++;
        if ((*errCount) > MAX_ERRORS)   //�������������
        {
            *stat = eYM_RECE_ERR;
        }
    }
}

int YmodemReceive(char *pRece, int *sReceLen, char *pData, int *sResLen)
{
    int cRes = YM_VOIDER;
    static eYM_STAT stat= eYM_INIT;                 //��ǰ״̬
    
    static char affirmCount = 0;                    //ȷ�ϴ���    
    static int iNumber      = 0;                    //���ݰ����
    static int sErrorCount  = 0;                    //���������
    static int sRecCount    = 0;                    //���մ���
    s8 cTmp = 0;
    
    switch (stat)
    {
    case eYM_INIT:
        iNumber = 0;
        sErrorCount = 0;   
        affirmCount = 0;
        stat = eYM_RECE_HEAD_PACKET;
        IS_TIMEOUT_1MS(eTimYModem, 0);          //�峬ʱ������
        break;

    case eYM_RECE_HEAD_PACKET:
        if (IS_TIMEOUT_1MS(eTimYModem, NAK_TIMEOUT))            //���ȴ���ʱ 
        {         
            YmodemSendChar(CRC16, &stat, &sErrorCount);         //���� 'C'
            stat = eYM_INIT;
            break;
        }
            
        if (*sReceLen == 0)
            break;
  
        cTmp = ReceivePacket(pRece, *sReceLen, pData, sResLen, iNumber & 0xFF);
        *sReceLen = 0;                                      //�������ݺ��������������
        switch (cTmp)
        {
        case 1:                                               
            if (pData[PACKET_HEADER] == 0)                  //���ļ�����
                stat = eYM_END;
            else
            {
                cRes = YM_FILE_INFO;                        //���ؽ���ͷ�ļ�
                stat = eYM_RECE_DATA_START;                 //���ļ�����
                iNumber++;       
            }
            break;
        case 2:                                             //�յ�EOT
            sRecCount++;
            if(sRecCount == 1)                              //��һ�λظ�NAK
            {
                SendString((u8 *)NAK,1);
            }
            if(sRecCount == 2)                              //�ڶ��λظ�ACK+C
            {
                sRecCount = 0;
                SendString((u8 *)ACK,1);
                SendString((u8 *)CRC16,1);
                stat = eYM_INIT;
            }
            break;
        case 0:                                             //���յ�������־
            stat = eYM_RECE_ERR;
            break;
        default:                                            //������������  
            YmodemSendChar(CRC16, &stat, &sErrorCount);
            if(affirmCount>=MAX_ERRORS)
            {
                stat = eYM_RECE_ERR;
            }
            affirmCount++;
            break;
        }
        IS_TIMEOUT_1MS(eTimYModem, 0);                      //�峬ʱ������
        break;
        
    case eYM_RECE_DATA_START:
        YmodemSendChar(ACK, &stat, &sErrorCount);           //��ȷӦ��
        YmodemSendChar(CRC16, &stat, &sErrorCount);         //���� 'C' 
        stat = eYM_RECE_DATA;
        break;
                
    case eYM_RECE_DATA:
        if (IS_TIMEOUT_1MS(eTimYModem, NAK_TIMEOUT))        //���ȴ���ʱ 
        {
            stat = eYM_END;
            break;
        }
            
        if (*sReceLen == 0)
            break;
  
        cTmp = ReceivePacket(pRece, *sReceLen, pData, sResLen, iNumber & 0xFF);
        *sReceLen = 0;                  //�������ݺ��������������
        switch (cTmp)
        {
        case 1:                         //������ȷ
            cRes = YM_FILE_DATA;        //���ؽ���������ȷ
            iNumber++;                  
            affirmCount = 0;
            YmodemSendChar(ACK, &stat, &sErrorCount);        //��ȷӦ��
            break;

        case 0:                         //���յ�������־
            if (affirmCount)
            {
                iNumber = 0;
                stat = eYM_RECE_HEAD_PACKET;
                YmodemSendChar(ACK, &stat, &sErrorCount);    //��ȷӦ��
            }
            else
            {
                YmodemSendChar(NAK, &stat, &sErrorCount);    //����Ӧ��
            }
            affirmCount++;
            break;

        default:                        //������������  
            affirmCount = 0;
            YmodemSendChar(NAK, &stat, &sErrorCount);       //����Ӧ��
            break;
        } 
        IS_TIMEOUT_1MS(eTimYModem, 0);                      //�峬ʱ������        
        break;
    case eYM_RECE_ERR:
        SendString((u8 *)CA,1);
        SendString((u8 *)CA,1);
        cRes = YM_EXIT;
        stat = eYM_INIT;
    case eYM_END:
        SendString((u8 *)ACK,1);           //��ֹ
    
        cRes = YM_EXIT;
        stat = eYM_INIT;
        break;
        
    }
    
    return cRes;
}





