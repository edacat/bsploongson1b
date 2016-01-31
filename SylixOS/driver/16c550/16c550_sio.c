/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: 16c550_sio.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 08 月 27 日
**
** 描        述: 16C550 SIO 驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include <SylixOS.h>
#include <driver/sio/16c550.h>
#include "16c550_sio_priv.h"
/*********************************************************************************************************
  16C550 SIO 通道
*********************************************************************************************************/
static SIO16C550_CHAN   _G_sio16C550Chans[BSP_CFG_16C550_SIO_NR];
/*********************************************************************************************************
** 函数名称: __sio16C550SetReg
** 功能描述: 设置 16C550 寄存器
** 输　入  : pSio16C550Chan        16C550 SIO 通道
**           iReg                  寄存器
**           ucValue               值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __sio16C550SetReg (SIO16C550_CHAN  *pSio16C550Chan, INT  iReg, UINT8  ucValue)
{
    SIO16C550_CFG   *pSio16C550Cfg = pSio16C550Chan->priv;

    write8(ucValue, pSio16C550Cfg->CFG_ulBase + iReg);
}
/*********************************************************************************************************
** 函数名称: __sio16C550GetReg
** 功能描述: 获得 16C550 寄存器的值
** 输　入  : pSio16C550Chan        16C550 SIO 通道
**           iReg                  寄存器
** 输　出  : 寄存器的值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  __sio16C550GetReg (SIO16C550_CHAN  *pSio16C550Chan, INT  iReg)
{
    SIO16C550_CFG   *pSio16C550Cfg = pSio16C550Chan->priv;

    return  (read8(pSio16C550Cfg->CFG_ulBase + iReg));
}
/*********************************************************************************************************
** 函数名称: __sio16C550Isr
** 功能描述: 16C550 中断服务程序
** 输　入  : pSio16C550Chan        16C550 SIO 通道
**           ulVector              中断向量号
** 输　出  : 中断返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  __sio16C550Isr (SIO16C550_CHAN  *pSio16C550Chan, ULONG  ulVector)
{
    sio16c550Isr(pSio16C550Chan);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: sioChan16C550Create
** 功能描述: 创建一个 SIO 通道
** 输　入  : uiChannel                 硬件通道号
** 输　出  : SIO 通道
** 全局变量:
** 调用模块:
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
                               "16c550_isr");                           /*  安装操作系统中断向量表      */

        API_InterVectorEnable(pSio16C550Cfg->CFG_ulVector);

        return  ((SIO_CHAN *)pSio16C550Chan);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
