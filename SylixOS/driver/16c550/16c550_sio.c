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
** ��   ��   ��: 16c550_sio.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 08 �� 27 ��
**
** ��        ��: 16C550 SIO ����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include <SylixOS.h>
#include <driver/sio/16c550.h>
#include "16c550_sio_priv.h"
/*********************************************************************************************************
  16C550 SIO ͨ��
*********************************************************************************************************/
static SIO16C550_CHAN   _G_sio16C550Chans[BSP_CFG_16C550_SIO_NR];
/*********************************************************************************************************
** ��������: __sio16C550SetReg
** ��������: ���� 16C550 �Ĵ���
** �䡡��  : pSio16C550Chan        16C550 SIO ͨ��
**           iReg                  �Ĵ���
**           ucValue               ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sio16C550SetReg (SIO16C550_CHAN  *pSio16C550Chan, INT  iReg, UINT8  ucValue)
{
    SIO16C550_CFG   *pSio16C550Cfg = pSio16C550Chan->priv;

    write8(ucValue, pSio16C550Cfg->CFG_ulBase + iReg);
}
/*********************************************************************************************************
** ��������: __sio16C550GetReg
** ��������: ��� 16C550 �Ĵ�����ֵ
** �䡡��  : pSio16C550Chan        16C550 SIO ͨ��
**           iReg                  �Ĵ���
** �䡡��  : �Ĵ�����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  __sio16C550GetReg (SIO16C550_CHAN  *pSio16C550Chan, INT  iReg)
{
    SIO16C550_CFG   *pSio16C550Cfg = pSio16C550Chan->priv;

    return  (read8(pSio16C550Cfg->CFG_ulBase + iReg));
}
/*********************************************************************************************************
** ��������: __sio16C550Isr
** ��������: 16C550 �жϷ������
** �䡡��  : pSio16C550Chan        16C550 SIO ͨ��
**           ulVector              �ж�������
** �䡡��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  __sio16C550Isr (SIO16C550_CHAN  *pSio16C550Chan, ULONG  ulVector)
{
    sio16c550Isr(pSio16C550Chan);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: sioChan16C550Create
** ��������: ����һ�� SIO ͨ��
** �䡡��  : uiChannel                 Ӳ��ͨ����
** �䡡��  : SIO ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
SIO_CHAN  *sioChan16C550Create (UINT  uiChannel)
{
    SIO16C550_CHAN          *pSio16C550Chan;
    SIO16C550_CFG           *pSio16C550Cfg;

    if (uiChannel < BSP_CFG_16C550_SIO_NR) {

        pSio16C550Chan = &_G_sio16C550Chans[uiChannel];
        pSio16C550Cfg  = &_G_sio16C550Cfgs[uiChannel];

        /*
         *  Receiver FIFO Trigger Level and Tirgger bytes table
         *  level  16 Bytes FIFO Trigger   32 Bytes FIFO Trigger  64 Bytes FIFO Trigger
         *    0              1                       8                    1
         *    1              4                      16                   16
         *    2              8                      24                   32
         *    3             14                      28                   56
         */
        pSio16C550Chan->fifo_len         = 16;
        pSio16C550Chan->rx_trigger_level = 3;

        pSio16C550Chan->baud   = pSio16C550Cfg->CFG_ulBaud;
        pSio16C550Chan->xtal   = pSio16C550Cfg->CFG_ulXtal;
        pSio16C550Chan->setreg = __sio16C550SetReg;
        pSio16C550Chan->getreg = __sio16C550GetReg;

        pSio16C550Chan->priv   = pSio16C550Cfg;

        API_InterVectorDisable(pSio16C550Cfg->CFG_ulVector);

        sio16c550Init(pSio16C550Chan);

        API_InterVectorConnect(pSio16C550Cfg->CFG_ulVector,
                               (PINT_SVR_ROUTINE)__sio16C550Isr,
                               (PVOID)pSio16C550Chan,
                               "16c550_isr");                           /*  ��װ����ϵͳ�ж�������      */

        API_InterVectorEnable(pSio16C550Cfg->CFG_ulVector);

        return  ((SIO_CHAN *)pSio16C550Chan);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
