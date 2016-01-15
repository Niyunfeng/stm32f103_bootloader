#include "YModem.h"

#define SendString(a, b)    BspUsart1Send(a, b)     //串口发送函数 a:字符串 b:长度

static unsigned short YModemCrc(char *pData, unsigned short sLen)
{  
   unsigned short CRC_DATA = 0;    //
   unsigned short i = 0;
   
   while (sLen--)  //len是所要计算的长度
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
    int len = 0;            //数据的长度
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
        return 2;               //接收到结束标志
    }
    else if (pRece[0] == ETX)   
    {
        return 0;               //接收到中止标志 ctrl + c
    }
    else if (pRece[0] == CA)    //接收到中止标志
    {
        if (pRece[1] == CA)     //确认中止标志
            return 0; 
        else
            return -6;          //接收到中止错误标志
    }
    else
    {
        return -1;              //数据头错误
    }

    if (pRece[1] != cNum)
        return -2;              //数据编号错误
     
    if (pRece[2] != (char)(~cNum))
        return -3;              //数据编号补码错误

    if (sReceLen != (len + PACKET_OVERHEAD))
        return -4;              //数据总长度错误

    // YModem只效验数据部分
    tmp = YModemCrc(pRece + PACKET_HEADER, len);
    if (((pRece[sReceLen - 2] << 8) |pRece[sReceLen - 1]) != tmp)
        return -5;              //crc效验错误

    memcpy(pData, &pRece[PACKET_HEADER], len);  //保存数据
    *sLen = len;

    return 1;
}


void YmodemSendChar(unsigned char ch, eYM_STAT *stat, int *errCount)
{
    while(TRUE != SendString(&ch, 1));
 
    if (ch == ACK)                      //错误计数器清零
    {
        *errCount = 0;
    }
    else if (ch == NAK)                 //错误计数器累加
    {
        (*errCount)++;
        if ((*errCount) > MAX_ERRORS)   //超过最大错误次数
        {
            *stat = eYM_RECE_ERR;
        }
    }
}

int YmodemReceive(char *pRece, int *sReceLen, char *pData, int *sResLen)
{
    int cRes = YM_VOIDER;
    static eYM_STAT stat= eYM_INIT;                 //当前状态
    
    static char affirmCount = 0;                    //确认次数    
    static int iNumber      = 0;                    //数据包编号
    static int sErrorCount  = 0;                    //错误包次数
    static int sRecCount    = 0;                    //接收次数
    s8 cTmp = 0;
    
    switch (stat)
    {
    case eYM_INIT:
        iNumber = 0;
        sErrorCount = 0;   
        affirmCount = 0;
        stat = eYM_RECE_HEAD_PACKET;
        IS_TIMEOUT_1MS(eTimYModem, 0);          //清超时计数器
        break;

    case eYM_RECE_HEAD_PACKET:
        if (IS_TIMEOUT_1MS(eTimYModem, NAK_TIMEOUT))            //若等待超时 
        {         
            YmodemSendChar(CRC16, &stat, &sErrorCount);         //发送 'C'
            stat = eYM_INIT;
            break;
        }
            
        if (*sReceLen == 0)
            break;
  
        cTmp = ReceivePacket(pRece, *sReceLen, pData, sResLen, iNumber & 0xFF);
        *sReceLen = 0;                                      //读完数据后，允许接收新数据
        switch (cTmp)
        {
        case 1:                                               
            if (pData[PACKET_HEADER] == 0)                  //无文件发送
                stat = eYM_END;
            else
            {
                cRes = YM_FILE_INFO;                        //返回接收头文件
                stat = eYM_RECE_DATA_START;                 //有文件发送
                iNumber++;       
            }
            break;
        case 2:                                             //收到EOT
            sRecCount++;
            if(sRecCount == 1)                              //第一次回复NAK
            {
                SendString((u8 *)NAK,1);
            }
            if(sRecCount == 2)                              //第二次回复ACK+C
            {
                sRecCount = 0;
                SendString((u8 *)ACK,1);
                SendString((u8 *)CRC16,1);
                stat = eYM_INIT;
            }
            break;
        case 0:                                             //接收到结束标志
            stat = eYM_RECE_ERR;
            break;
        default:                                            //接收数据有误  
            YmodemSendChar(CRC16, &stat, &sErrorCount);
            if(affirmCount>=MAX_ERRORS)
            {
                stat = eYM_RECE_ERR;
            }
            affirmCount++;
            break;
        }
        IS_TIMEOUT_1MS(eTimYModem, 0);                      //清超时计数器
        break;
        
    case eYM_RECE_DATA_START:
        YmodemSendChar(ACK, &stat, &sErrorCount);           //正确应答
        YmodemSendChar(CRC16, &stat, &sErrorCount);         //发送 'C' 
        stat = eYM_RECE_DATA;
        break;
                
    case eYM_RECE_DATA:
        if (IS_TIMEOUT_1MS(eTimYModem, NAK_TIMEOUT))        //若等待超时 
        {
            stat = eYM_END;
            break;
        }
            
        if (*sReceLen == 0)
            break;
  
        cTmp = ReceivePacket(pRece, *sReceLen, pData, sResLen, iNumber & 0xFF);
        *sReceLen = 0;                  //读完数据后，允许接收新数据
        switch (cTmp)
        {
        case 1:                         //接收正确
            cRes = YM_FILE_DATA;        //返回接收数据正确
            iNumber++;                  
            affirmCount = 0;
            YmodemSendChar(ACK, &stat, &sErrorCount);        //正确应答
            break;

        case 0:                         //接收到结束标志
            if (affirmCount)
            {
                iNumber = 0;
                stat = eYM_RECE_HEAD_PACKET;
                YmodemSendChar(ACK, &stat, &sErrorCount);    //正确应答
            }
            else
            {
                YmodemSendChar(NAK, &stat, &sErrorCount);    //错误应答
            }
            affirmCount++;
            break;

        default:                        //接收数据有误  
            affirmCount = 0;
            YmodemSendChar(NAK, &stat, &sErrorCount);       //错误应答
            break;
        } 
        IS_TIMEOUT_1MS(eTimYModem, 0);                      //清超时计数器        
        break;
    case eYM_RECE_ERR:
        SendString((u8 *)CA,1);
        SendString((u8 *)CA,1);
        cRes = YM_EXIT;
        stat = eYM_INIT;
    case eYM_END:
        SendString((u8 *)ACK,1);           //中止
    
        cRes = YM_EXIT;
        stat = eYM_INIT;
        break;
        
    }
    
    return cRes;
}





