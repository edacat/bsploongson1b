/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: ls1x_i2c.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 12 �� 14 ��
**
** ��        ��: LS1X I2C ����(������������)
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "../clock/ls1x_clock.h"
#include <linux/compat.h>
/*********************************************************************************************************
  ���Ͷ���
*********************************************************************************************************/
#define I2C_CHANNEL_NR          (2)
#define I2C_BUS_FREQ_MAX        (400)
#define I2C_BUS_FREQ_MIN        (100)
#define LS1X_I2C0_REGS          (0xBFE58000)
#define LS1X_I2C1_REGS          (0xBFE68000)

/* registers */
#define LS1X_I2C_PRELOW         (0x00)
#define LS1X_I2C_PREHIGH        (0x01)
#define LS1X_I2C_CONTROL        (0x02)
#define LS1X_I2C_DATA           (0x03)
#define LS1X_I2C_CMD            (0x04)                                  /* write only                   */
#define LS1X_I2C_STATUS         (0x04)                                  /* read  only                   */

#define LS1XI2C_CTRL_IEN        (0x40)
#define LS1XI2C_CTRL_EN         (0x80)

#define LS1XI2C_CMD_START       (0x90)
#define LS1XI2C_CMD_STOP        (0x40)
#define LS1XI2C_CMD_READ        (0x20)
#define LS1XI2C_CMD_WRITE       (0x10)
#define LS1XI2C_CMD_READ_ACK    (0x20)
#define LS1XI2C_CMD_READ_NACK   (0x28)
#define LS1XI2C_CMD_IACK        (0x00)

#define LS1XI2C_STAT_IF         (0x01)
#define LS1XI2C_STAT_TIP        (0x02)
#define LS1XI2C_STAT_ARBLOST    (0x20)
#define LS1XI2C_STAT_BUSY       (0x40)
#define LS1XI2C_STAT_NACK       (0x80)

/*********************************************************************************************************
  i2c ͨ�����Ͷ���
*********************************************************************************************************/
typedef struct {
    addr_t              I2CCHAN_ulPhyAddrBase;                          /*  �����ַ����ַ              */
    UINT                I2CCHAN_uiBusFreq;

    LW_HANDLE           I2CCHAN_hSignal;                                /*  ͬ���ź���                  */
    BOOL                I2CCHAN_bIsInit;                                /*  �Ƿ��Ѿ���ʼ��              */
    PUCHAR              I2CCHAN_pucBuffer;
    UINT                I2CCHAN_uiBufferLen;
} LS1X_I2C_CHANNEL, *PLS1X_I2C_CHANNEL;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LS1X_I2C_CHANNEL  _G_ls1xI2cChannels[I2C_CHANNEL_NR] = {
        {
                LS1X_I2C0_REGS,
                I2C_BUS_FREQ_MIN,
        },
        {
                LS1X_I2C1_REGS,
                I2C_BUS_FREQ_MIN,
        },
};

/*********************************************************************************************************
** ��������: ls1xI2PollStatus
** ��������: Check Status Register Bit
** �䡡��  : pI2cChannel i2c ͨ��
**           INT8        iBit ״̬�Ĵ���һλ
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT ls1xI2PollStatus(PLS1X_I2C_CHANNEL  pI2cChannel, INT8 iBit)
{
    INT     iCount = 20 *1000;

    do {
        API_TimeSleep(LW_OPTION_WAIT_A_TICK);
    } while ((readb(pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_STATUS) & iBit) && (--iCount > 0));

    return (iCount > 0);
}

/*********************************************************************************************************
** ��������: ls1xI2cXferRead
** ��������: I2C ����ʱ�� Read
** �䡡��  : pI2cChannel i2c ͨ��
**           UINT8       pBuf Read Buffer
**           INT         iLength Read Lenggth
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT ls1xI2cXferRead(PLS1X_I2C_CHANNEL  pI2cChannel, UINT8 *pBuf, INT iLength)
{
    INT     iIndex;

    for (iIndex = 0; iIndex < iLength; iIndex++) {
        /* send ACK last not send ACK */
        if (iIndex != (iLength - 1)) {
            writeb(LS1XI2C_CMD_READ_ACK, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);
        }
        else {
            writeb(LS1XI2C_CMD_READ_NACK, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);
        }

        if (!ls1xI2PollStatus(pI2cChannel, LS1XI2C_STAT_TIP)) {
            return  (PX_ERROR);
        }
        *pBuf++ = readb(pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_DATA);
    }

    writeb(LS1XI2C_CMD_STOP, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xI2cXferWrite
** ��������: I2C ����ʱ�� Write
** �䡡��  : pI2cChannel i2c ͨ��
**           UINT8       pBuf Write Buffer
**           INT         iLength Write Lenggth
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT ls1xI2cXferWrite(PLS1X_I2C_CHANNEL  pI2cChannel, UINT8 *pBuf, INT iLength)
{
    INT     iIndex;

    for (iIndex = 0; iIndex < iLength; iIndex++) {
        writeb(*pBuf++, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_DATA);
        writeb(LS1XI2C_CMD_WRITE, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);
        if (!ls1xI2PollStatus(pI2cChannel, LS1XI2C_STAT_TIP)) {
            return  (PX_ERROR);
        }
        if (readb(pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_STATUS) & LS1XI2C_STAT_NACK) {
            writeb(LS1XI2C_CMD_STOP, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);
            return iLength;
        }
    }
    writeb(LS1XI2C_CMD_STOP, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xI2cTryTransfer
** ��������: i2c ���Դ��亯��
** �䡡��  : pI2cChannel     i2c ͨ��
**           pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xI2cTryTransfer (PLS1X_I2C_CHANNEL  pI2cChannel,
                                PLW_I2C_ADAPTER    pI2cAdapter,
                                PLW_I2C_MESSAGE    pI2cMsg,
                                INT                iNum)
{
    INT     iIndex, iRet;

    for (iIndex = 0; iIndex < iNum; iIndex++, pI2cMsg++) {

        if (!ls1xI2PollStatus(pI2cChannel, LS1XI2C_STAT_BUSY)) {
            return  (PX_ERROR);
        }

        writeb((pI2cMsg->I2CMSG_usAddr << 1) | ((pI2cMsg->I2CMSG_usFlag & LW_I2C_M_RD) ? 1 : 0),
                pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_DATA);

        writeb(LS1XI2C_CMD_START, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);

        /* Wait until transfer is finished */
        if (!ls1xI2PollStatus(pI2cChannel, LS1XI2C_STAT_TIP)) {
            return (PX_ERROR);
        }

        if (readb(pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_STATUS) & LS1XI2C_STAT_NACK) {
            writeb(LS1XI2C_CMD_STOP, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);
            return (ERROR_NONE);
        }

        if (pI2cMsg->I2CMSG_usFlag & LW_I2C_M_RD)
            iRet = ls1xI2cXferRead(pI2cChannel, pI2cMsg->I2CMSG_pucBuffer, pI2cMsg->I2CMSG_usLen);
        else
            iRet = ls1xI2cXferWrite(pI2cChannel, pI2cMsg->I2CMSG_pucBuffer, pI2cMsg->I2CMSG_usLen);

        if (iRet)
            return iRet;
    }

    return  (iIndex);
}
/*********************************************************************************************************
** ��������: ls1xI2cTransfer
** ��������: i2c ���亯��
** �䡡��  : pI2cChannel     i2c ͨ��
**           pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xI2cTransfer (PLS1X_I2C_CHANNEL  pI2cChannel,
                             PLW_I2C_ADAPTER    pI2cAdapter,
                             PLW_I2C_MESSAGE    pI2cMsg,
                             INT                iNum)
{
    REGISTER INT        i;

    for (i = 0; i < pI2cAdapter->I2CADAPTER_iRetry; i++) {
        if (ls1xI2cTryTransfer(pI2cChannel, pI2cAdapter, pI2cMsg, iNum) == iNum) {
            return  (iNum);
        } else {
            API_TimeSleep(LW_OPTION_WAIT_A_TICK);                       /*  �ȴ�һ��������������        */
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls1xI2c0Transfer
** ��������: i2c0 ���亯��
** �䡡��  : pI2cAdapter     i2c ������
**           pI2cMsg         i2c ������Ϣ��
**           iNum            ��Ϣ����
** �䡡��  : ��ɴ������Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xI2c0Transfer (PLW_I2C_ADAPTER     pI2cAdapter,
                                  PLW_I2C_MESSAGE     pI2cMsg,
                                  INT                 iNum)
{
    return  (ls1xI2cTransfer(&_G_ls1xI2cChannels[0], pI2cAdapter, pI2cMsg, iNum));
}

/*********************************************************************************************************
** ��������: ls1xI2cMasterCtl
** ��������: i2c ���ƺ���
** �䡡��  : pI2cChannel     i2c ͨ��
**           pI2cAdapter     i2c ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xI2cMasterCtl (PLS1X_I2C_CHANNEL  pI2cChannel,
                              PLW_I2C_ADAPTER    pI2cAdapter,
                              INT                iCmd,
                              LONG               lArg)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xI2c0MasterCtl
** ��������: i2c0 ���ƺ���
** �䡡��  : pI2cAdapter     i2c ������
**           iCmd            ����
**           lArg            ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xI2c0MasterCtl(PLW_I2C_ADAPTER   pI2cAdapter,
                              INT               iCmd,
                              LONG              lArg)
{
    return  (ls1xI2cMasterCtl(&_G_ls1xI2cChannels[0], pI2cAdapter, iCmd, lArg));
}

/*********************************************************************************************************
** ��������: ls1xI2cHwInit
** ��������: ��ʼ�� i2c Ӳ��ͨ��
** �䡡��  : pI2cChannel     i2c ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xI2cHwInit(PLS1X_I2C_CHANNEL  pI2cChannel)
{
    INT   iPrescale = 0;

    UINT8 uiCtrl = readb(pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CONTROL);

    /* make sure the device is disabled */
    writeb(uiCtrl & ~(LS1XI2C_CTRL_EN | LS1XI2C_CTRL_IEN),
            pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CONTROL);
    return  (ERROR_NONE);

    iPrescale = ls1xAPBClock();

    iPrescale = iPrescale / (5 * pI2cChannel->I2CCHAN_uiBusFreq) - 1;

    writeb((iPrescale  & 0xFF), pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_PRELOW);
    writeb((iPrescale >> 0x08), pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_PREHIGH);

    /* Init the device */
    writeb(LS1XI2C_CMD_IACK, pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CMD);
    writeb((uiCtrl | LS1XI2C_CTRL_IEN | LS1XI2C_CTRL_EN),
            pI2cChannel->I2CCHAN_ulPhyAddrBase + LS1X_I2C_CONTROL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xI2cInit
** ��������: ��ʼ�� i2c ͨ��
** �䡡��  : pI2cChannel     i2c ͨ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xI2cInit(PLS1X_I2C_CHANNEL  pI2cChannel)
{
    if (!pI2cChannel->I2CCHAN_bIsInit) {

        pI2cChannel->I2CCHAN_hSignal = API_SemaphoreBCreate("i2c_signal",
                                                            LW_FALSE,
                                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pI2cChannel->I2CCHAN_hSignal == LW_OBJECT_HANDLE_INVALID) {
            printk(KERN_ERR "ls1xI2cInit(): failed to create i2c_signal!\n");
            goto  error_handle;
        }

        if (ls1xI2cHwInit(pI2cChannel) != ERROR_NONE) {
            printk(KERN_ERR "ls1xI2cInit(): failed to init!\n");
            goto  error_handle;
        }

        pI2cChannel->I2CCHAN_bIsInit = LW_TRUE;
    }

    return  (ERROR_NONE);

error_handle:

    if (pI2cChannel->I2CCHAN_hSignal) {
        API_SemaphoreBDelete(&pI2cChannel->I2CCHAN_hSignal);
        pI2cChannel->I2CCHAN_hSignal = 0;
    }

    return  (PX_ERROR);
}

static LW_I2C_FUNCS  _G_ls1xI2cFuncs[I2C_CHANNEL_NR] = {
        {
                ls1xI2c0Transfer,
                ls1xI2c0MasterCtl,
        },
};
/*********************************************************************************************************
** ��������: i2cBusFuns
** ��������: ��ʼ�� i2c ���߲���ȡ��������
** �䡡��  : iChan     ͨ����
** �䡡��  : i2c ������������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_I2C_FUNCS  i2cBusFuns (UINT  uiChannel)
{
    if (uiChannel >= I2C_CHANNEL_NR) {
        printk(KERN_ERR "i2cBusFuns(): I2C channel invalid!\n");
        return  (LW_NULL);
    }

    if (ls1xI2cInit(&_G_ls1xI2cChannels[uiChannel]) != ERROR_NONE) {
        return  (LW_NULL);
    }

    return  (&_G_ls1xI2cFuncs[uiChannel]);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

