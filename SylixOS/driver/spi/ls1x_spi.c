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
** 文   件   名: ls1x_spi.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 12 月 20 日
**
** 描        述: Loongson1B 处理器 SPI 驱动(总线驱动部分)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "../clock/ls1x_clock.h"
#include "ls1x_spi.h"
#include <linux/compat.h>
#include <sys/ioccom.h>
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define LS1X_SPI_CHANNEL_NR     (2)
#define LS1X_SPI0_BASE          (0xBFE80000)
#define LS1X_SPI1_BASE          (0xBFEC0000)
#define LS1X_SPI0_IRQ           BSP_CFG_INT0_SUB_VECTOR(8)
#define LS1X_SPI1_IRQ           BSP_CFG_INT0_SUB_VECTOR(9)
/*********************************************************************************************************
  SPI 寄存器定义
*********************************************************************************************************/
#define LS1X_SPI_SPCR           (0x00)                                  /*  控制寄存器                  */
#define LS1X_SPI_SPSR           (0x01)                                  /*  状态寄存器                  */
#define LS1X_SPI_TXFIFO         (0x02)                                  /*  数据传输寄存器 输出         */
#define LS1X_SPI_RXFIFO         (0x02)                                  /*  数据传输寄存器 输入         */
#define LS1X_SPI_SPER           (0x03)                                  /*  外部寄存器                  */
#define LS1X_SPI_PARAM          (0x04)                                  /*  SPI Flash参数控制寄存器     */
#define LS1X_SPI_SOFTCS         (0x05)                                  /*  SPI Flash片选控制寄存器     */
#define LS1X_SPI_TIMING         (0x06)                                  /*  SPI Flash时序控制寄存器     */

#define SPI_SET_CSD_CMD         (_IOW('i', 0, INT))
#define SPI_SET_CSE_CMD         (_IOW('i', 1, INT))
#define SPI_SET_CLK_CMD         (_IOW('i', 2, INT))

#define SPIDEVICE_NAME_SIZE     (32)
/*********************************************************************************************************
  SPI Device Infomation, SylixOS 兼容SPI Bus多个Slave device, Bus 定义好device的驱动
*********************************************************************************************************/
typedef struct {
    UCHAR                       SPIDEV_ucModalias[SPIDEVICE_NAME_SIZE];
    CPVOID                      SPIDEV_pvPlatformData;
    PVOID                       SPIDEV_pvControllerData;
    INT                         SPIDEV_iVic;
    UINT32                      SPIDEV_uiMaxSpeedHz;
    UINT16                      SPIDEV_uiBusNum;
    UINT16                      SPIDEV_uiChipSelect;
    UINT8                       SPIDEV_uiMode;
} LS1XSPI_DEVICEINFO, *PLS1XSPI_DEVICEINFO;
/*********************************************************************************************************
  SPI 通道类型定义
*********************************************************************************************************/
typedef struct {
    addr_t                      SPICHAN_ulPhyAddrBase;                  /*  物理地址基地址              */
    UINT                        SPICHAN_uiChannel;                      /*  通道号                      */
    INT                         SPICHAN_iVic;                           /*  SPI 总线中断号              */
    UINT32                      SPICHAN_uiClk;                          /*  SPI 总线Clock               */
    UINT32                      SPICHAN_uiSpeed;                        /*  SPI 总线Speed               */
    UINT8                       SPICHAN_uiMode;                         /*  SPI 总线Mode                */
    LW_HANDLE                   SPICHAN_hSignal;                        /*  同步信号量                  */
    BOOL                        SPICHAN_bIsInit;                        /*  是否已经初始化              */
} LS1X_SPI_CHANNEL, *PLS1X_SPI_CHANNEL;

/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LS1X_SPI_CHANNEL  _G_ls1xSpiChannels[LS1X_SPI_CHANNEL_NR] = {
        {
                LS1X_SPI0_BASE,
                LS1XSPI0,
                LS1X_SPI0_IRQ,
                0,
                6000000,
                0
        },
        {
                LS1X_SPI1_BASE,
                LS1XSPI1,
                LS1X_SPI1_IRQ,
                0,
                6000000,
                0
        },
};

static LS1XSPI_DEVICEINFO _G_ls1xSPI0_DeviceInfo[] = {
        {
                .SPIDEV_ucModalias   = "m25p80",
                .SPIDEV_uiBusNum     = 0,
                .SPIDEV_uiChipSelect = SPI_CS0,
                .SPIDEV_uiMaxSpeedHz = 400000,
        },
        {
                .SPIDEV_ucModalias   = "ads7846",
                .SPIDEV_uiBusNum     = 0,
                .SPIDEV_uiChipSelect = SPI_CS1,
                .SPIDEV_uiMaxSpeedHz = 400000,
                .SPIDEV_uiMode       = 0,
        },
        {
                .SPIDEV_ucModalias   = "mmc_spi",
                .SPIDEV_uiBusNum     = 0,
                .SPIDEV_uiChipSelect = SPI_CS2,
                .SPIDEV_uiMaxSpeedHz = 400000,
                .SPIDEV_uiMode       = 3,
        },
        {
                .SPIDEV_ucModalias   = "mcp3201",
                .SPIDEV_uiBusNum     = 0,
                .SPIDEV_uiChipSelect = SPI_CS3,
                .SPIDEV_uiMaxSpeedHz = 400000,
        },
};

static  UINT    _G_uiActive_MasterID;                                   /*  当前激活的Bus Number        */
static  UINT    _G_uiActive_DeviceID;                                   /*  当前激活的Bus Number        */

/*********************************************************************************************************
** 函数名称: ls1xSpiDiv
** 功能描述: 计算SPI Divier
** 输　入  : pSpiChannel     spi 通道
** 输　出  : vivier因子
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT ls1xSpiDiv(PLS1X_SPI_CHANNEL  pSpiChannel)
{
    INT     iDiv, iDivTmp, iBit;
    UINT32  uiClk, uiSpeed;

    uiClk   = pSpiChannel->SPICHAN_uiClk;
    uiSpeed = pSpiChannel->SPICHAN_uiSpeed;

    iDiv    = DIV_ROUND_UP(uiClk, uiSpeed);

    if (iDiv < 2) {
        iDiv = 2;
    }


    if (iDiv > 4096) {
        iDiv = 4096;
    }

    iBit = archFindMsb(iDiv) - 1;

    switch(1 << iBit) {
    case 16:
        iDivTmp = 2;
        if (iDiv > (1 << iBit)) {
            iDivTmp++;
        }
        break;

    case 32:
        iDivTmp = 3;
        if (iDiv > (1 << iBit)) {
            iDivTmp += 2;
        }
        break;

    case 8:
        iDivTmp = 4;
        if (iDiv > (1 << iBit)) {
            iDivTmp -= 2;
        }
        break;

    default:
        iDivTmp = iBit - 1;
        if (iDiv > (1 << iBit)) {
            iDivTmp++;
        }
        break;
    }

    return iDivTmp;
}
/*********************************************************************************************************
** 函数名称: SetSPIClk
** 功能描述: Config SPI Clock & Speed
** 输　入  : pSpiChannel     spi 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiSetClk(PLS1X_SPI_CHANNEL  pSpiChannel)
{
    INT     iDiv;
    UINT8   uiReg;

    iDiv    = ls1xSpiDiv(pSpiChannel);

    pSpiChannel->SPICHAN_uiMode &= (LW_SPI_M_CPOL_1 | LW_SPI_M_CPHA_1);

    uiReg   = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPCR);
    uiReg  &= 0xF0;
    uiReg  |= (pSpiChannel->SPICHAN_uiMode << 2) | (iDiv & 0x03);
    writeb(uiReg, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPCR);

    uiReg   = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPER);
    uiReg  &= 0xFC;
    uiReg  |= (iDiv >> 2);
    writeb(uiReg, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPER);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xSpiWaitTxE
** 功能描述: spi 传输消息TX Enable
** 输　入  : pSpiChannel     spi 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiWaitTxE (PLS1X_SPI_CHANNEL  pSpiChannel)
{
    INT     iTimeout = 20000;

    while (iTimeout--) {
        if (readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPSR) & 0x80) {
            break;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xSpiWaitRxE
** 功能描述: spi 传输消息RX Enable
** 输　入  : pSpiChannel     spi 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiWaitRxE (PLS1X_SPI_CHANNEL  pSpiChannel)
{
    UINT8   uiVal;

    uiVal   = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPSR);
    uiVal  |= 0x80;
    writeb(uiVal, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPSR);  /*  Int Clear                   */
    uiVal   = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPSR);
    if (uiVal & 0x40) {
        /* Write-Collision Clear */
        writeb(uiVal & 0xBF, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPSR);
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xSpiTransferMsg
** 功能描述: spi 传输消息
** 输　入  : pSpiChannel     spi 通道
**           pSpiMsg         spi 传输消息
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiTransferMsg (PLS1X_SPI_CHANNEL  pSpiChannel,
                                PLW_SPI_MESSAGE    pSpiMsg)
{
    UINT32  uiIndex;
    UINT8  *pucWrBuf;
    UINT8  *pucRdBuf;

    if ((pSpiMsg->SPIMSG_pucWrBuffer == LW_NULL &&
         pSpiMsg->SPIMSG_pucRdBuffer == LW_NULL) ||
         pSpiMsg->SPIMSG_uiLen == 0) {
        return  (PX_ERROR);
    }

    pucWrBuf    = pSpiMsg->SPIMSG_pucWrBuffer;
    pucRdBuf    = pSpiMsg->SPIMSG_pucRdBuffer;

    if (pSpiMsg->SPIMSG_pucWrBuffer && pSpiMsg->SPIMSG_pucRdBuffer) {
        for (uiIndex = 0; uiIndex < pSpiMsg->SPIMSG_uiLen; uiIndex++) {
            if (pSpiMsg->SPIMSG_usFlag & LW_SPI_M_WRBUF_FIX) {
                writeb(*pucWrBuf, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_TXFIFO);
            } else {
                writeb(*pucWrBuf++, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_TXFIFO);
            }
            ls1xSpiWaitTxE(pSpiChannel);

            if (pSpiMsg->SPIMSG_usFlag & LW_SPI_M_RDBUF_FIX) {
                *pucRdBuf = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_RXFIFO);
            } else {
                *pucRdBuf++ = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_RXFIFO);
            }
            ls1xSpiWaitRxE(pSpiChannel);
        }
    } else if (pSpiMsg->SPIMSG_pucRdBuffer) {
        for (uiIndex = 0; uiIndex < pSpiMsg->SPIMSG_uiLen; uiIndex++) {
            writeb(0, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_TXFIFO);
            ls1xSpiWaitTxE(pSpiChannel);

            *pucRdBuf++ = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_RXFIFO);
            ls1xSpiWaitRxE(pSpiChannel);
        }
    } else if (pSpiMsg->SPIMSG_pucWrBuffer) {
        for (uiIndex = 0; uiIndex < pSpiMsg->SPIMSG_uiLen; uiIndex++) {
            writeb(*pucWrBuf++, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_TXFIFO);
            ls1xSpiWaitTxE(pSpiChannel);

            readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_RXFIFO);
            ls1xSpiWaitRxE(pSpiChannel);
        }
    } else {
        for (uiIndex = 0; uiIndex < pSpiMsg->SPIMSG_uiLen; uiIndex++) {
            writeb(0, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_TXFIFO);
            ls1xSpiWaitTxE(pSpiChannel);

            *pucRdBuf++ = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_RXFIFO);
            ls1xSpiWaitRxE(pSpiChannel);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xSpiTransfer
** 功能描述: spi 传输函数
** 输　入  : pSpiChannel     spi 通道
**           pSpiAdapter     spi 适配器
**           pSpiMsg         spi 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiTransfer (PLS1X_SPI_CHANNEL  pSpiChannel,
                             PLW_SPI_ADAPTER    pSpiAdapter,
                             PLW_SPI_MESSAGE    pSpiMsg,
                             INT                iNum)
{
    REGISTER INT        i;
    INT                 iError;

    for (i = 0; i < iNum; i++, pSpiMsg++) {
        iError = ls1xSpiTransferMsg(pSpiChannel, pSpiMsg);
        if (iError != ERROR_NONE) {
            return  (i);
        }
    }

    return  (iNum);
}
/*********************************************************************************************************
** 函数名称: ls1xSpi0Transfer
** 功能描述: spi0 传输函数
** 输　入  : pSpiAdapter     spi 适配器
**           pSpiMsg         spi 传输消息组
**           iNum            消息数量
** 输　出  : 完成传输的消息数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpi0Transfer (PLW_SPI_ADAPTER     pSpiAdapter,
                              PLW_SPI_MESSAGE     pSpiMsg,
                              INT                 iNum)
{
    return  (ls1xSpiTransfer(&_G_ls1xSpiChannels[0], pSpiAdapter, pSpiMsg, iNum));
}

/*********************************************************************************************************
** 函数名称: ls1xSpiChipSelect
** 功能描述: 初始化 spi 硬件通道
** 输　入  : pSpiChannel     spi 通道
**         : iCSValue        Chip Select Value
**         : bEnable         Chip Select Disable or Enable
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT ls1xSpiChipSelect(PLS1X_SPI_CHANNEL  pSpiChannel, INT iCSValue, BOOL bEnable)
{
    UINT    uiVal;

    uiVal   = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SOFTCS);
    uiVal   = (uiVal & 0XF0) | (0X01 << iCSValue);

    if (bEnable) {
        uiVal   = uiVal & (~(0X10 << iCSValue));
        writeb(uiVal, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SOFTCS);
    } else {
        uiVal   = uiVal | (0X10 << iCSValue);
        writeb(uiVal, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SOFTCS);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xSpiMasterCtl
** 功能描述: spi 控制函数
** 输　入  : pSpiChannel     spi 通道
**           pSpiAdapter     spi 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiMasterCtl (PLS1X_SPI_CHANNEL  pSpiChannel,
                              PLW_SPI_ADAPTER    pSpiAdapter,
                              INT                iCmd,
                              LONG               lArg)
{
    INT     iRet = ERROR_NONE;

    if (!pSpiChannel) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Invalid adapter address.\r\n");
        return  (PX_ERROR);
    }

    switch (iCmd) {
    case LW_SPI_CTL_BAUDRATE:
        _G_ls1xSPI0_DeviceInfo[_G_uiActive_DeviceID].SPIDEV_uiMaxSpeedHz = (UINT32)lArg;
        pSpiChannel->SPICHAN_uiSpeed = (UINT32)lArg;
        break;

    case LW_SPI_CTL_USERFUNC:
        iRet = PX_ERROR;
        break;

    default:
        iRet = PX_ERROR;
        break;

    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: ls1xSpi0MasterCtl
** 功能描述: spi0 控制函数
** 输　入  : pSpiAdapter     spi 适配器
**           iCmd            命令
**           lArg            参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpi0MasterCtl (PLW_SPI_ADAPTER   pSpiAdapter,
                               INT               iCmd,
                               LONG              lArg)
{
    return  (ls1xSpiMasterCtl(&_G_ls1xSpiChannels[0], pSpiAdapter, iCmd, lArg));
}

/*********************************************************************************************************
** 函数名称: ls1xSpiHwInit
** 功能描述: 初始化 spi 硬件通道
** 输　入  : pSpiChannel     spi 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiHwInit(PLS1X_SPI_CHANNEL  pSpiChannel)
{
    UINT8   uiVal;

    /* 计算SPI CLK*/
    pSpiChannel->SPICHAN_uiClk = ls1xAPBClock();
    /* 使能SPI控制器，master模式，使能或关闭中断 */
    writeb(0x53, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPCR);
    /* 清空状态寄存器 */
    writeb(0xC0, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPSR);
    /* 1字节产生中断，采样(读)与发送(写)时机同时 */
    writeb(0x03, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SPER);
    /* 片选设置 */
    writeb(0xFF, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_SOFTCS);
    /* 关闭SPI flash */
    uiVal  = readb(pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_PARAM);
    uiVal &= 0xFE;
    writeb(uiVal, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_PARAM);
    /* SPI flash时序控制寄存器 */
    writeb(0x05, pSpiChannel->SPICHAN_ulPhyAddrBase + LS1X_SPI_TIMING);

    ls1xSpiSetClk(pSpiChannel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: SPIPO_pfuncChipSelect
** 功能描述: 初始化 spi 总线并获取驱动程序
** 输　入  : iMasterID  BUS Number
**         : iDeviceID  Slave Device CS
**         : bEnable    Enable or Disable Device
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID ls1xSpipfuncChipSelect(UINT  iMasterID, UINT iDeviceID, BOOL bEnable)
{
    PLS1X_SPI_CHANNEL   pSpiChannel     = &_G_ls1xSpiChannels[iMasterID];
    PLS1XSPI_DEVICEINFO pSpiDeviceInfo  = &_G_ls1xSPI0_DeviceInfo[iDeviceID];

    pSpiChannel->SPICHAN_uiSpeed = pSpiDeviceInfo->SPIDEV_uiMaxSpeedHz;
    pSpiChannel->SPICHAN_uiMode  = pSpiDeviceInfo->SPIDEV_uiMode;

    if (bEnable) {
        _G_uiActive_MasterID = iMasterID;
        _G_uiActive_DeviceID = iDeviceID;
        ls1xSpiSetClk(pSpiChannel);
    }

    ls1xSpiChipSelect(pSpiChannel, pSpiDeviceInfo->SPIDEV_uiChipSelect, bEnable);
}

/*********************************************************************************************************
** 函数名称: ls1xSpiInit
** 功能描述: 初始化 spi 通道
** 输　入  : pSpiChannel     spi 通道
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xSpiInit(PLS1X_SPI_CHANNEL  pSpiChannel)
{
    if (!pSpiChannel->SPICHAN_bIsInit) {
        pSpiChannel->SPICHAN_hSignal = API_SemaphoreCCreate("spi_signal",
                                                            0,
                                                            3,
                                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pSpiChannel->SPICHAN_hSignal == LW_OBJECT_HANDLE_INVALID) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "ls1xSpiInit(): failed to create spi_signal!\r\n");
                API_SemaphoreCDelete(&pSpiChannel->SPICHAN_hSignal);
                pSpiChannel->SPICHAN_hSignal = 0;

                return  (PX_ERROR);
            }

        if (ls1xSpiHwInit(pSpiChannel) != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "ls1xSpiInit(): failed to init!\r\n");
                API_SemaphoreCDelete(&pSpiChannel->SPICHAN_hSignal);
                pSpiChannel->SPICHAN_hSignal = 0;

                return  (PX_ERROR);
            }

            pSpiChannel->SPICHAN_bIsInit = LW_TRUE;
    }
    return  (ERROR_NONE);
}

/*********************************************************************************************************
  Loongson1B 处理器 spi 驱动程序
*********************************************************************************************************/
static LW_SPI_FUNCS  _G_ls1xSpiFuncs[LS1X_SPI_CHANNEL_NR] = {
        {
                ls1xSpi0Transfer,
                ls1xSpi0MasterCtl,
        }
};
/*********************************************************************************************************
** 函数名称: spiBusDrv
** 功能描述: 初始化 spi 总线并获取驱动程序
** 输　入  : uiChannel         通道号
** 输　出  : spi 总线驱动程序
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_SPI_FUNCS  spiBusDrv (UINT  uiChannel)
{
    if (uiChannel >= LS1X_SPI_CHANNEL_NR) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "spiBusDrv(): spi channel invalid!\r\n");
        return  (LW_NULL);
    }

    if (ls1xSpiInit(&_G_ls1xSpiChannels[uiChannel]) != ERROR_NONE) {
        return  (LW_NULL);
    }

    return  (&_G_ls1xSpiFuncs[uiChannel]);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
