/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: designware_eth.c
**
** 创   建   人: Jiao.JinXing
**
** 文件创建日期: 2015 年 10 月 29 日
**
** 描        述: designware 以太网芯片驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include <linux/compat.h>
#include "designware_eth.h"

#define CONFIG_DW_GMAC_DEFAULT_DMA_PBL  16
#include "designware_eth_priv.h"
/*********************************************************************************************************
  配置
*********************************************************************************************************/
#define IFNAME0                 'e'
#define IFNAME1                 'n'

#define HOSTNAME                "lwIP"

#define CONFIG_TX_DESCR_NUM     32
#define CONFIG_RX_DESCR_NUM     32
#define CONFIG_ETH_BUFSIZE      2048
#define TX_TOTAL_BUFSIZE        (CONFIG_ETH_BUFSIZE * CONFIG_TX_DESCR_NUM)
#define RX_TOTAL_BUFSIZE        (CONFIG_ETH_BUFSIZE * CONFIG_RX_DESCR_NUM)
/*********************************************************************************************************
  MAC 寄存器定义
*********************************************************************************************************/
typedef struct {
    UINT32                      MAC_uiConfig;                           /*  0x00                        */
    UINT32                      MAC_uiFrameFilt;                        /*  0x04                        */
    UINT32                      MAC_uiHashTableHigh;                    /*  0x08                        */
    UINT32                      MAC_uiHashTableLow;                     /*  0x0c                        */
    UINT32                      MAC_uiMiiAddr;                          /*  0x10                        */
    UINT32                      MAC_uiMiiData;                          /*  0x14                        */
    UINT32                      MAC_uiFlowControl;                      /*  0x18                        */
    UINT32                      MAC_uiVlanTag;                          /*  0x1c                        */
    UINT32                      MAC_uiVersion;                          /*  0x20                        */
    UINT8                       reserved_1[20];
    UINT32                      MAC_uiIntReg;                           /*  0x38                        */
    UINT32                      MAC_uiIntMask;                          /*  0x3c                        */
    UINT32                      MAC_uiMacAddr0Hi;                       /*  0x40                        */
    UINT32                      MAC_uiMacAddr0Lo;                       /*  0x44                        */
} __ETH_MAC_REGS, *__PETH_MAC_REGS;
/*********************************************************************************************************
  DMA 寄存器定义
*********************************************************************************************************/
typedef struct {
    UINT32                      DMA_uiBusMode;                          /*  0x00                        */
    UINT32                      DMA_uiTxPollDemand;                     /*  0x04                        */
    UINT32                      DMA_uiRxPollDemand;                     /*  0x08                        */
    UINT32                      DMA_uiRxDescListAddr;                   /*  0x0c                        */
    UINT32                      DMA_uiTxDescListAddr;                   /*  0x10                        */
    UINT32                      DMA_uiStatus;                           /*  0x14                        */
    UINT32                      DMA_uiOpMode;                           /*  0x18                        */
    UINT32                      DMA_uiIntEnable;                        /*  0x1c                        */
    UINT32                      reserved1[2];
    UINT32                      DMA_uiAxiBus;                           /*  0x28                        */
    UINT32                      reserved2[7];
    UINT32                      DMA_uiCurrHostTxDesc;                   /*  0x48                        */
    UINT32                      DMA_uiCurrHostRxDesc;                   /*  0x4c                        */
    UINT32                      DMA_uiCurrHostTxBuffAddr;               /*  0x50                        */
    UINT32                      DMA_uiCurrHostRxBuffAddr;               /*  0x54                        */
} __ETH_DMA_REGS, *__PETH_DMA_REGS;
/*********************************************************************************************************
  DMA MAC 描述符定义
*********************************************************************************************************/
struct __ST_DMA_MAC_DESCR {
    volatile UINT32             DESCR_uiTxRxStatus;
    volatile UINT32             DESCR_uiCntl;
    PVOID                       DESCR_pvAddr;
    struct __ST_DMA_MAC_DESCR  *DESCR_pNest;
} __attribute__ ((aligned(ARCH_DMA_MINALIGN)));
typedef struct __ST_DMA_MAC_DESCR __DMA_MAC_DESCR, *__PDMA_MAC_DESCR;
/*********************************************************************************************************
  以太网接口数据结构定义
*********************************************************************************************************/
typedef struct {
    addr_t                      ETH_ulBase;

    __PETH_DMA_REGS             ETH_pDmaRegs;
    __PETH_MAC_REGS             ETH_pMacRegs;
    __PDMA_MAC_DESCR            ETH_pTxDmaDescrTable;
    __PDMA_MAC_DESCR            ETH_pRxDmaDescrTable;
    UINT8                      *ETH_pucTxDmaBuffers;
    UINT8                      *ETH_pucRxDmaBuffers;
    UINT32                      ETH_uiTxDmaDescIn;
    UINT32                      ETH_uiTxDmaDescOut;
    atomic_t                    ETH_atomicTxDmaDescCnt;
    UINT32                      ETH_uiRxCurrDescNum;

    LW_OBJECT_HANDLE            ETH_hTxDmaDescrSem;

    DESIGNWARE_ETH_DATA         ETH_platfromData;

    spinlock_t                  ETH_spinlock;

    PHY_DRV_FUNC                ETH_phyDrvFunc;
    PHY_DEV                     ETH_phyDev;
} __DESIGNWARE_ETH, *__PDESIGNWARE_ETH;

static __PDESIGNWARE_ETH        _G_apDesignwareEth[2] = { LW_NULL, LW_NULL };
/*********************************************************************************************************
** 函数名称: __designwareEthMiiPhyRead
** 功能描述: PHY 读
** 输　入  : ucAddr        PHY 设备地址(0 - 31)
**           ucReg         PHY 寄存器
**           usVal         值
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEthMiiPhyRead (__PDESIGNWARE_ETH   pDesignwareEth,
                                       UCHAR  ucAddr, UCHAR  ucReg, UINT16  *pusVal)
{
    __PETH_MAC_REGS     pMacRegs       = pDesignwareEth->ETH_pMacRegs;
    UINT16              usMiiAddr;
    INT                 iReTryCnt      = 10000;

    usMiiAddr = ((ucAddr << MIIADDRSHIFT) & MII_ADDRMSK) |
                ((ucReg  << MIIREGSHIFT)  & MII_REGMSK);

    write32(usMiiAddr | MII_CLKRANGE_100_150M | __MII_BUSY,
            (addr_t)&pMacRegs->MAC_uiMiiAddr);

    while (--iReTryCnt > 0) {
        if (!(read32((addr_t)&pMacRegs->MAC_uiMiiAddr) & __MII_BUSY)) {
            *pusVal = read32((addr_t)&pMacRegs->MAC_uiMiiData);
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __designwareEthMiiPhyWrite
** 功能描述: PHY 写
** 输　入  : ucAddr        PHY 设备地址(0 - 31)
**           ucReg         PHY 寄存器
**           usVal         值
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEthMiiPhyWrite (__PDESIGNWARE_ETH   pDesignwareEth,
                                        UCHAR  ucAddr, UCHAR  ucReg, UINT16  usVal)
{
    __PETH_MAC_REGS     pMacRegs       = pDesignwareEth->ETH_pMacRegs;
    UINT16              usMiiAddr;
    INT                 iReTryCnt      = 10000;

    write32(usVal, (addr_t)&pMacRegs->MAC_uiMiiData);

    usMiiAddr = ((ucAddr << MIIADDRSHIFT) & MII_ADDRMSK) |
                ((ucReg  << MIIREGSHIFT ) & MII_REGMSK)  | __MII_WRITE;

    write32(usMiiAddr | MII_CLKRANGE_100_150M | __MII_BUSY,
            (addr_t)&pMacRegs->MAC_uiMiiAddr);

    while (--iReTryCnt > 0) {
        if (!(read32((addr_t)&pMacRegs->MAC_uiMiiAddr) & __MII_BUSY)) {
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __designwareEth0MiiPhyRead
** 功能描述: PHY 读
** 输　入  : ucAddr        PHY 设备地址(0 - 31)
**           ucReg         PHY 寄存器
**           usVal         值
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEth0MiiPhyRead (UCHAR  ucAddr, UCHAR  ucReg, UINT16  *pusVal)
{
    return  (__designwareEthMiiPhyRead(_G_apDesignwareEth[0], ucAddr, ucReg, pusVal));
}
/*********************************************************************************************************
** 函数名称: __designwareEth0MiiPhyWrite
** 功能描述: PHY 写
** 输　入  : ucAddr        PHY 设备地址(0 - 31)
**           ucReg         PHY 寄存器
**           usVal         值
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEth0MiiPhyWrite (UCHAR  ucAddr, UCHAR  ucReg, UINT16  usVal)
{
    return  (__designwareEthMiiPhyWrite(_G_apDesignwareEth[0], ucAddr, ucReg, usVal));
}
/*********************************************************************************************************
** 函数名称: __designwareEth1MiiPhyRead
** 功能描述: PHY 读
** 输　入  : ucAddr        PHY 设备地址(0 - 31)
**           ucReg         PHY 寄存器
**           usVal         值
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEth1MiiPhyRead (UCHAR  ucAddr, UCHAR  ucReg, UINT16  *pusVal)
{
    return  (__designwareEthMiiPhyRead(_G_apDesignwareEth[1], ucAddr, ucReg, pusVal));
}
/*********************************************************************************************************
** 函数名称: __designwareEth1MiiPhyWrite
** 功能描述: PHY 写
** 输　入  : ucAddr        PHY 设备地址(0 - 31)
**           ucReg         PHY 寄存器
**           usVal         值
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEth1MiiPhyWrite (UCHAR  ucAddr, UCHAR  ucReg, UINT16  usVal)
{
    return  (__designwareEthMiiPhyWrite(_G_apDesignwareEth[1], ucAddr, ucReg, usVal));
}
/*********************************************************************************************************
** 函数名称: __designwareEthMiiLinkStatus
** 功能描述: MII 连接状态变化
** 输　入  : pNetif        网卡接口
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEthMiiLinkStatus (struct netif  *pNetif)
{
    __PDESIGNWARE_ETH   pDesignwareEth = (__PDESIGNWARE_ETH)pNetif->state;
    __PETH_MAC_REGS     pMacRegs       = pDesignwareEth->ETH_pMacRegs;
    PHY_DEV            *pPhyDev        = &pDesignwareEth->ETH_phyDev;
    UINT32              uiConfig;

    uiConfig = read32((addr_t)&pMacRegs->MAC_uiConfig) | FRAMEBURSTENABLE | DISABLERXOWN;

    if (pPhyDev->PHY_usPhyStatus & MII_SR_LINK_STATUS) {

        if (pPhyDev->PHY_uiPhySpeed != MII_1000MBS) {
            uiConfig |= MII_PORTSELECT;
        }

        if (pPhyDev->PHY_uiPhySpeed == MII_100MBS) {
            uiConfig |= FES_100;
        }

        if (pPhyDev->PHY_uiPhyFlags & MII_PHY_FD) {
            uiConfig |= FULLDPLXMODE;
        }

        write32(uiConfig, (addr_t)&pMacRegs->MAC_uiConfig);

        pNetif->link_speed = pPhyDev->PHY_uiPhySpeed;
        netif_set_link_up(pNetif);
    } else {
        netif_set_link_down(pNetif);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __designwareEthWriteHwAddr
** 功能描述: 写 MAC 地址
** 输　入  : NONE
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEthWriteHwAddr (__PDESIGNWARE_ETH  pDesignwareEth, UINT8  *pucMacAddr)
{
    __PETH_MAC_REGS     pMacRegs = pDesignwareEth->ETH_pMacRegs;
    UINT32              uiMacIdLo;
    UINT32              uiMacIdHi;

    uiMacIdLo = pucMacAddr[0] + (pucMacAddr[1] << 8) + (pucMacAddr[2] << 16) + (pucMacAddr[3] << 24);
    uiMacIdHi = pucMacAddr[4] + (pucMacAddr[5] << 8);

    write32(uiMacIdHi, (addr_t)&pMacRegs->MAC_uiMacAddr0Hi);
    write32(uiMacIdLo, (addr_t)&pMacRegs->MAC_uiMacAddr0Lo);

    write32(0xffffffff, (addr_t)&pMacRegs->MAC_uiHashTableHigh);
    write32(0xffffffff, (addr_t)&pMacRegs->MAC_uiHashTableLow);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __designwareEthTxDmaDescrInit
** 功能描述: 初始化发送 DMA 描述符
** 输　入  : NONE
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEthTxDmaDescrInit (__PDESIGNWARE_ETH  pDesignwareEth)
{
    __PETH_DMA_REGS     pDmaRegs       = pDesignwareEth->ETH_pDmaRegs;
    __PDMA_MAC_DESCR    pDmaDescrTable = &pDesignwareEth->ETH_pTxDmaDescrTable[0];
    __PDMA_MAC_DESCR    pDmaDescr;
    UINT8              *pucBuffer      = &pDesignwareEth->ETH_pucTxDmaBuffers[0];
    INT                 i;

    /*
     * 发送 DMA 描述符表里的 DMA 描述符组织成环形链表
     */
    for (i = 0; i < CONFIG_TX_DESCR_NUM; i++) {
        pDmaDescr = &pDmaDescrTable[i];
        pDmaDescr->DESCR_pvAddr = &pucBuffer[i * CONFIG_ETH_BUFSIZE];
        pDmaDescr->DESCR_pNest = &pDmaDescrTable[i + 1];

#if defined(CONFIG_DW_ALTDESCRIPTOR)
        pDmaDescr->txrx_status &= ~(DESC_TXSTS_TXINT | DESC_TXSTS_TXLAST |
                DESC_TXSTS_TXFIRST | DESC_TXSTS_TXCRCDIS | \
                DESC_TXSTS_TXCHECKINSCTRL | \
                DESC_TXSTS_TXRINGEND | DESC_TXSTS_TXPADDIS);

        pDmaDescr->txrx_status |= DESC_TXSTS_TXCHAIN;
        pDmaDescr->dmamac_cntl  = 0;
        pDmaDescr->txrx_status &= ~(DESC_TXSTS_MSK | DESC_TXSTS_OWNBYDMA);
#else
        pDmaDescr->DESCR_uiCntl = DESC_TXCTRL_TXCHAIN;
        pDmaDescr->DESCR_uiTxRxStatus = 0;
#endif
    }
    pDmaDescr->DESCR_pNest = &pDmaDescrTable[0];

    KN_IO_WMB();
    write32((UINT32)&pDmaDescrTable[0],
            (addr_t)&pDmaRegs->DMA_uiTxDescListAddr);                   /*  写入发送 DMA 描述符环链地址 */

    pDesignwareEth->ETH_uiTxDmaDescIn  = 0;                             /*  发送 DMA 描述符入队点       */
    pDesignwareEth->ETH_uiTxDmaDescOut = 0;                             /*  发送 DMA 描述符出队点       */
    API_AtomicSet(0, &pDesignwareEth->ETH_atomicTxDmaDescCnt);          /*  发送 DMA 描述符队列计数     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __designwareEthRxDmaDecsrInit
** 功能描述: 初始化接收 DMA 描述符
** 输　入  : NONE
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEthRxDmaDecsrInit (__PDESIGNWARE_ETH  pDesignwareEth)
{
    __PETH_DMA_REGS     pDmaRegs       = pDesignwareEth->ETH_pDmaRegs;
    __PDMA_MAC_DESCR    pDmaDescrTable = &pDesignwareEth->ETH_pRxDmaDescrTable[0];
    __PDMA_MAC_DESCR    pDmaDescr;
    UINT8              *pucBuffer      = &pDesignwareEth->ETH_pucRxDmaBuffers[0];
    INT                 i;

    /*
     * 接收 DMA 描述符表里的 DMA 描述符组织成环形链表
     */
    for (i = 0; i < CONFIG_RX_DESCR_NUM; i++) {
        pDmaDescr = &pDmaDescrTable[i];
        pDmaDescr->DESCR_pvAddr = &pucBuffer[i * CONFIG_ETH_BUFSIZE];
        pDmaDescr->DESCR_pNest = &pDmaDescrTable[i + 1];

        pDmaDescr->DESCR_uiCntl = (MAC_MAX_FRAME_SZ & DESC_RXCTRL_SIZE1MASK) | DESC_RXCTRL_RXCHAIN;

        pDmaDescr->DESCR_uiTxRxStatus = DESC_RXSTS_OWNBYDMA;
    }
    pDmaDescr->DESCR_pNest = &pDmaDescrTable[0];

    KN_IO_WMB();
    write32((UINT32)&pDmaDescrTable[0],
            (addr_t)&pDmaRegs->DMA_uiRxDescListAddr);                   /*  写入接收 DMA 描述符环链地址 */

    pDesignwareEth->ETH_uiRxCurrDescNum = 0;                            /*  当前接收 DMA 描述符编号为 0 */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __designwareEthInit
** 功能描述: 初始化控制器
** 输　入  : NONE
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __designwareEthInit (struct netif  *pNetif)
{
    __PDESIGNWARE_ETH   pDesignwareEth = pNetif->state;
    __PETH_MAC_REGS     pMacRegs       = pDesignwareEth->ETH_pMacRegs;
    __PETH_DMA_REGS     pDmaRegs       = pDesignwareEth->ETH_pDmaRegs;
    PHY_DEV            *pPhyDev        = &pDesignwareEth->ETH_phyDev;
    PHY_DRV_FUNC       *pPhyDrvFunc    = &pDesignwareEth->ETH_phyDrvFunc;

    write32(read32((addr_t)&pDmaRegs->DMA_uiBusMode) | DMAMAC_SRST,
            (addr_t)&pDmaRegs->DMA_uiBusMode);                          /*  复位                        */

    while (read32((addr_t)&pDmaRegs->DMA_uiBusMode) & DMAMAC_SRST) {    /*  等待复位完成                */
    }

    __designwareEthWriteHwAddr(pDesignwareEth, pNetif->hwaddr);         /*  设置硬件地址                */

    __designwareEthRxDmaDecsrInit(pDesignwareEth);                      /*  初始化接收 DMA 描述符       */
    __designwareEthTxDmaDescrInit(pDesignwareEth);                      /*  初始化发送 DMA 描述符       */

    write32(FIXEDBURST | PRIORXTX_41 | DMA_PBL,
            (addr_t)&pDmaRegs->DMA_uiBusMode);

#ifndef CONFIG_DW_MAC_FORCE_THRESHOLD_MODE
    write32(read32((addr_t)&pDmaRegs->DMA_uiOpMode) | FLUSHTXFIFO | STOREFORWARD,
            (addr_t)&pDmaRegs->DMA_uiOpMode);
#else
    write32(read32((addr_t)&pDmaRegs->opmode) | FLUSHTXFIFO,
            (addr_t)&pDmaRegs->opmode);
#endif

    write32(read32((addr_t)&pDmaRegs->DMA_uiOpMode) | RXSTART | TXSTART,
            (addr_t)&pDmaRegs->DMA_uiOpMode);

#ifdef CONFIG_DW_AXI_BURST_LEN
    write32((CONFIG_DW_AXI_BURST_LEN & 0x1FF >> 1), (addr_t)&pDmaRegs->axibus);
#endif

    write32(read32((addr_t)&pMacRegs->MAC_uiConfig) | RXENABLE | TXENABLE,
            (addr_t)&pMacRegs->MAC_uiConfig);                           /*  使能发送与接收              */

    write32(DMA_INTR_DEFAULT_MASK,
            (addr_t)&pDmaRegs->DMA_uiIntEnable);                        /*  打开发送接收中断            */

    if (_G_apDesignwareEth[0] == pDesignwareEth) {
        pPhyDrvFunc->PHYF_pfuncWrite    = (FUNCPTR)__designwareEth0MiiPhyWrite;
        pPhyDrvFunc->PHYF_pfuncRead     = (FUNCPTR)__designwareEth0MiiPhyRead;
    } else {
        pPhyDrvFunc->PHYF_pfuncWrite    = (FUNCPTR)__designwareEth1MiiPhyWrite;
        pPhyDrvFunc->PHYF_pfuncRead     = (FUNCPTR)__designwareEth1MiiPhyRead;
    }

    pPhyDrvFunc->PHYF_pfuncLinkDown     = (FUNCPTR)__designwareEthMiiLinkStatus;

    pPhyDev->PHY_pPhyDrvFunc = pPhyDrvFunc;
    pPhyDev->PHY_pvMacDrv    = pNetif;
    pPhyDev->PHY_ucPhyAddr   = 0;
    pPhyDev->PHY_uiPhyID     = pDesignwareEth->ETH_platfromData.uiPhyId;
    pPhyDev->PHY_uiTryMax    = 100;
    pPhyDev->PHY_uiLinkDelay = 100;                                     /*  延时100毫秒自动协商过程     */
    pPhyDev->PHY_uiPhyFlags  = MII_PHY_AUTO |                           /*  自动协商标志                */
#if 0
                               MII_PHY_1000T_FD |                       /*  PHY may use 1000-T full dup */
                               MII_PHY_1000T_HD |                       /*  PHY may use 1000-T half dup */
#endif
                               MII_PHY_100 |                            /*  PHY may use 100Mbit speed   */
                               MII_PHY_10 |                             /*  PHY may use 10Mbit speed    */
                               MII_PHY_FD |                             /*  PHY may use full duplex     */
                               MII_PHY_HD |                             /*  PHY may use half duplex     */
                               MII_PHY_MONITOR;                         /*  启用自动监视功能            */

    API_MiiPhyInit(pPhyDev);                                            /*  初始化 PHY                  */

    API_MiiPhyMonitorStart();                                           /*  开始监控 PHY 状态           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __designwareEthRx
** 功能描述: 接收
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __designwareEthRx (struct netif  *pNetif)
{
    __PDESIGNWARE_ETH   pDesignwareEth = pNetif->state;
    __PETH_DMA_REGS     pDmaRegs       = pDesignwareEth->ETH_pDmaRegs;
    __PDMA_MAC_DESCR    pDmaDescr;
    UINT32              uiStatus;
    UINT32              uiTotalLen;
    struct pbuf        *pBuf;
    err_t               err;
    INT                 iPacketNr;
    INT                 iMaxReTryCnt;

    iPacketNr    = 0;
    iMaxReTryCnt = 1000;

    while (1) {
                                                                        /*  当前接收 DMA 描述符         */
        pDmaDescr = &pDesignwareEth->ETH_pRxDmaDescrTable[pDesignwareEth->ETH_uiRxCurrDescNum];

        KN_IO_WMB();
        uiStatus  = pDmaDescr->DESCR_uiTxRxStatus;                      /*  接收状态                    */

        if (!(uiStatus & DESC_RXSTS_OWNBYDMA)) {                        /*  接收完成                    */

            iPacketNr++;

            uiTotalLen = (uiStatus & DESC_RXSTS_FRMLENMSK)
                          >> DESC_RXSTS_FRMLENSHFT;                     /*  报文总长度                  */
#if ETH_PAD_SIZE
            uiTotalLen += ETH_PAD_SIZE;                                 /*  调整报文总长度              */
#endif
            pBuf = pbuf_alloc(PBUF_RAW, uiTotalLen, PBUF_POOL);         /*  分配 PBUF                   */
            if (pBuf != NULL) {
#if ETH_PAD_SIZE
                pbuf_header(pBuf, -ETH_PAD_SIZE);                       /*  向后调整报文首部            */
#endif

                pbuf_take(pBuf,
                          pDmaDescr->DESCR_pvAddr,
                          uiTotalLen - ETH_PAD_SIZE);                   /*  拷贝接收缓冲区到 PBUF       */

                KN_IO_WMB();

                pDmaDescr->DESCR_uiTxRxStatus |= DESC_RXSTS_OWNBYDMA;   /*  释放当前接收 DMA 描述符     */
                KN_IO_WMB();

#if ETH_PAD_SIZE
                uiTotalLen -= ETH_PAD_SIZE;                             /*  调整报文总长度              */
                pbuf_header(pBuf, ETH_PAD_SIZE);                        /*  向前调整报文首部            */
#endif

                LINK_STATS_INC(link.recv);                              /*  修改统计信息                */
                snmp_add_ifinoctets(pNetif, uiTotalLen);
                snmp_inc_ifinucastpkts(pNetif);

                err = pNetif->input(pBuf, pNetif);                      /*  上传 PBUF 到上层            */
                if (err != ERR_OK) {
                    printk(KERN_ERR "__designwareEthRx(): packet input error, err = %d!\n", err);
                    pbuf_free(pBuf);
                    pBuf = NULL;
                }
            } else {                                                    /*  分配 PBUF 失败              */
                KN_IO_WMB();
                pDmaDescr->DESCR_uiTxRxStatus |= DESC_RXSTS_OWNBYDMA;   /*  释放当前接收 DMA 描述符     */
                KN_IO_WMB();

                LINK_STATS_INC(link.memerr);                            /*  修改统计信息                */
                LINK_STATS_INC(link.drop);
                snmp_inc_ifindiscards(pNetif);
            }

            pDesignwareEth->ETH_uiRxCurrDescNum++;                      /*  修正当前接收 DMA 描述符编号 */
            if (pDesignwareEth->ETH_uiRxCurrDescNum >= CONFIG_RX_DESCR_NUM) {
                pDesignwareEth->ETH_uiRxCurrDescNum = 0;
            }
        } else {
            if (iPacketNr > 0 || iMaxReTryCnt <= 0) {                   /*  如果接收过包 OR 延时过      */
                break;
            } else {
                iMaxReTryCnt--;
            }
        }
    }

    write32(DMA_INTR_DEFAULT_MASK,                                      /*  打开接收中断                */
            (addr_t)&pDmaRegs->DMA_uiIntEnable);

    if (iPacketNr == 0) {
        printk(KERN_ERR "__designwareEthRx(): no packet rx!\n");
    }
}
/*********************************************************************************************************
** 函数名称: __designwareEthTx
** 功能描述: 发送
** 输　入  : NONE
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static err_t  __designwareEthTx (struct netif  *pNetif, struct pbuf  *pBuf)
{
    __PDESIGNWARE_ETH   pDesignwareEth = pNetif->state;
    __PETH_DMA_REGS     pDmaRegs       = pDesignwareEth->ETH_pDmaRegs;
    __PDMA_MAC_DESCR    pDmaDescr;
    UINT32              uiTotalLen;
    INT                 iError;

    iError = API_SemaphoreCPend(pDesignwareEth->ETH_hTxDmaDescrSem,
                                LW_OPTION_WAIT_INFINITE);               /*  等待发送 DMA 描述符信号     */
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "__designwareEthTx(): wait timeout!\n");
        return  (ERR_TIMEOUT);
    }
                                                                        /*  发送 DMA 描述符             */
    pDmaDescr = &pDesignwareEth->ETH_pTxDmaDescrTable[pDesignwareEth->ETH_uiTxDmaDescIn];

    KN_IO_WMB();
    if (pDmaDescr->DESCR_uiTxRxStatus & DESC_TXSTS_OWNBYDMA) {          /*  仍被 DMA 占用               */
        printk(KERN_ERR "__designwareEthTx(): DMA own this tx frame!\n");
        API_SemaphoreCPost(pDesignwareEth->ETH_hTxDmaDescrSem);         /*  释放发送 DMA 描述符信号     */
        return  (ERR_BUF);
    }

    uiTotalLen = pBuf->tot_len;                                         /*  报文总长度                  */
#if ETH_PAD_SIZE
    uiTotalLen -= ETH_PAD_SIZE;                                         /*  向后调整长度和首部          */
    pbuf_header(pBuf, -ETH_PAD_SIZE);
#endif

    pbuf_copy_partial(pBuf, pDmaDescr->DESCR_pvAddr, uiTotalLen, 0);    /*  拷贝 PBUF 到发送缓冲区      */

#if ETH_PAD_SIZE
    pbuf_header(pBuf, ETH_PAD_SIZE);                                    /*  向前调整首部                */
#endif

#if defined(CONFIG_DW_ALTDESCRIPTOR)
    KN_IO_WMB();
    pDmaDescr->txrx_status |= DESC_TXSTS_TXFIRST | DESC_TXSTS_TXLAST;
    pDmaDescr->dmamac_cntl |= (uiTotalLen << DESC_TXCTRL_SIZE1SHFT) & \
                   DESC_TXCTRL_SIZE1MASK;

    pDmaDescr->txrx_status &= ~(DESC_TXSTS_MSK);
    pDmaDescr->txrx_status |= DESC_TXSTS_OWNBYDMA;
    KN_IO_WMB();
#else
    KN_IO_WMB();
    pDmaDescr->DESCR_uiCntl &= ~(DESC_TXCTRL_SIZE1MASK);
    pDmaDescr->DESCR_uiCntl |= ((uiTotalLen << DESC_TXCTRL_SIZE1SHFT) & \
                   DESC_TXCTRL_SIZE1MASK) | DESC_TXCTRL_TXLAST | \
                   DESC_TXCTRL_TXFIRST;

    pDmaDescr->DESCR_uiTxRxStatus = DESC_TXSTS_OWNBYDMA;
    KN_IO_WMB();
#endif

    pDesignwareEth->ETH_uiTxDmaDescIn++;                                /*  修正发送 DMA 描述符入队点   */
    if (pDesignwareEth->ETH_uiTxDmaDescIn >= CONFIG_TX_DESCR_NUM) {
        pDesignwareEth->ETH_uiTxDmaDescIn = 0;
    }

    API_AtomicInc(&pDesignwareEth->ETH_atomicTxDmaDescCnt);             /*  发送 DMA 描述符队列计数加一 */

    write32(POLL_DATA, (addr_t)&pDmaRegs->DMA_uiTxPollDemand);          /*  开始传输                    */

    LINK_STATS_INC(link.xmit);                                          /*  修改统计信息                */
    snmp_add_ifoutoctets(pNetif, uiTotalLen);
    snmp_inc_ifoutucastpkts(pNetif);

    return  (ERR_OK);
}
/*********************************************************************************************************
** 函数名称: __designwareEthIsr
** 功能描述: 中断服务程序
** 输　入  : NONE
** 输　出  : 中断返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  __designwareEthIsr (struct netif  *pNetif, ULONG  ulVector)
{
    __PDESIGNWARE_ETH   pDesignwareEth = pNetif->state;
    __PETH_DMA_REGS     pDmaRegs       = pDesignwareEth->ETH_pDmaRegs;
    __PDMA_MAC_DESCR    pDmaDescr;
    UINT32              uiTxDmaDescCnt;
    INT                 iPacketNr;
    INT                 iError;
    UINT32              uiIntStatus;
    UINT32              uiIntEnable;

    uiIntStatus = read32((addr_t)&pDmaRegs->DMA_uiStatus);              /*  中断状态                    */

    if (uiIntStatus & DMA_STATUS_AIS) {                                 /*  错误中断                    */
        printk(KERN_ERR "__designwareEthIsr(): ABNORMAL interrupt!\n");
    }

    if (uiIntStatus & DMA_STATUS_NIS) {                                 /*  正常中断                    */

        uiIntEnable = read32((addr_t)&pDmaRegs->DMA_uiIntEnable);
        if ((uiIntStatus & DMA_STATUS_RI) &&                            /*  接收中断                    */
            (uiIntEnable & DMA_INTR_ENA_RIE)) {

            pDmaDescr = &pDesignwareEth->ETH_pRxDmaDescrTable[pDesignwareEth->ETH_uiRxCurrDescNum];

            KN_IO_WMB();
            if (!(pDmaDescr->DESCR_uiTxRxStatus & DESC_RXSTS_OWNBYDMA)) {   /*  接收完成                */

                iError = netJobAdd((VOIDFUNCPTR)__designwareEthRx,      /*  增加一个接收工作            */
                                   pNetif,
                                   0, 0, 0, 0, 0);
                if (iError == ERROR_NONE) {
                    uiIntEnable &= ~DMA_INTR_ENA_RIE;
                    write32(uiIntEnable,                                /*  关闭接收中断                */
                            (addr_t)&pDmaRegs->DMA_uiIntEnable);
                } else {
                    printk(KERN_ERR "__designwareEthIsr(): failed to add net job!\n");
                }
            }
        }

        if ((uiIntStatus & DMA_STATUS_TI) ||
            (uiIntStatus & DMA_INTR_ENA_TUE)) {                         /*  发送中断                    */

                                                                        /*  发送 DMA 描述符队列计数     */
            uiTxDmaDescCnt = API_AtomicGet(&pDesignwareEth->ETH_atomicTxDmaDescCnt);
            if (uiTxDmaDescCnt > 0) {
                                                                        /*  发送 DMA 描述符             */
                pDmaDescr = &pDesignwareEth->ETH_pTxDmaDescrTable[pDesignwareEth->ETH_uiTxDmaDescOut];

                for (iPacketNr = 0; iPacketNr < uiTxDmaDescCnt; iPacketNr++) {

                    KN_IO_WMB();
                    if (!(pDmaDescr->DESCR_uiTxRxStatus & DESC_TXSTS_OWNBYDMA)) {
                                                                        /*  不被 DMA 占用               */

                        pDesignwareEth->ETH_uiTxDmaDescOut++;           /*  修正发送 DMA 描述符出队点   */
                        if (pDesignwareEth->ETH_uiTxDmaDescOut >= CONFIG_TX_DESCR_NUM) {
                            pDesignwareEth->ETH_uiTxDmaDescOut = 0;
                        }
                                                                        /*  发送 DMA 描述符             */
                        pDmaDescr = &pDesignwareEth->ETH_pTxDmaDescrTable\
                                    [pDesignwareEth->ETH_uiTxDmaDescOut];
                    } else {
                        break;
                    }
                }

                if (iPacketNr > 0) {
                    API_AtomicSub(iPacketNr,                            /*  发送 DMA 描述符队列计数减 i */
                                  &pDesignwareEth->ETH_atomicTxDmaDescCnt);

                    API_SemaphoreCRelease(pDesignwareEth->ETH_hTxDmaDescrSem,
                                          iPacketNr,
                                          LW_NULL);                     /*  释放 i 个发送 DMA 描述符信号*/
                }
            }
        }
    }

    write32((uiIntStatus & 0x1ffff), (addr_t)&pDmaRegs->DMA_uiStatus);  /*  清除中断                    */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: designwareEthInit
** 功能描述: 初始化
** 输　入  : NONE
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
err_t  designwareEthInit (struct netif  *pNetif)
{
    __PDESIGNWARE_ETH       pDesignwareEth;
    INT                     iError;

    LWIP_ASSERT("netif != NULL", (pNetif != NULL));

    if (pNetif->state == NULL) {                                        /*  平台数据检查                */
        printk(KERN_ERR "designwareEthInit(): invalid platfrom data!\n");
        return  (ERR_ARG);
    }
                                                                        /*  分配网卡数据结构            */
    pDesignwareEth = (__PDESIGNWARE_ETH)malloc(sizeof(__DESIGNWARE_ETH));
    if (pDesignwareEth == NULL) {
        printk(KERN_ERR "designwareEthInit(): failed to alloc memory!\n");
        return  (ERR_MEM);
    }
    lib_memset(pDesignwareEth, 0, sizeof(__DESIGNWARE_ETH));            /*  清零网卡数据结构            */

    if (_G_apDesignwareEth[0] == LW_NULL) {
        _G_apDesignwareEth[0] = pDesignwareEth;                         /*  记录网卡数据结构            */
    } else {                                                            /*  暂时只支持两个网卡          */
        _G_apDesignwareEth[1] = pDesignwareEth;
    }

    lib_memcpy(&pDesignwareEth->ETH_platfromData,
               pNetif->state,
               sizeof(DESIGNWARE_ETH_DATA));                            /*  拷贝平台数据                */

    pNetif->state = pDesignwareEth;                                     /*  记录网卡数据结构            */

#if LWIP_NETIF_HOSTNAME
    pNetif->hostname = HOSTNAME;                                        /*  设置主机名                  */
#endif /* LWIP_NETIF_HOSTNAME */

    pNetif->name[0] = IFNAME0;                                          /*  设置网卡名                  */
    pNetif->name[1] = IFNAME1;

    NETIF_INIT_SNMP(pNetif, snmp_ifType_ethernet_csmacd, 100000000);    /*  初始化 SNMP                 */

    pNetif->output     = etharp_output;                                 /*  安装驱动函数                */
    pNetif->linkoutput = __designwareEthTx;
    pNetif->output_ip6 = ethip6_output;

    lib_memcpy(pNetif->hwaddr,
               pDesignwareEth->ETH_platfromData.ucMacAddr,
               ETHARP_HWADDR_LEN);                                      /*  设置 MAC 地址               */

    pNetif->hwaddr_len = ETHARP_HWADDR_LEN;                             /*  设置 MAC 地址长度           */

    pNetif->mtu = 1500;                                                 /*  设置最大传输单元            */

    pNetif->flags = NETIF_FLAG_BROADCAST |
                    NETIF_FLAG_ETHARP    |
                    NETIF_FLAG_ETHERNET;                                /*  设置设备特性                */


    pDesignwareEth->ETH_ulBase = (addr_t)API_VmmIoRemapNocache(
                                         (PVOID)pDesignwareEth->ETH_platfromData.ulBase,
                                         2 * LW_CFG_VMM_PAGE_SIZE);     /*  重映射地址空间              */
    if (pDesignwareEth->ETH_ulBase == (addr_t)LW_NULL) {
        printk(KERN_ERR "designwareEthInit(): failed to remap memory space!\n");
        return  (ERR_MEM);
    }
    pDesignwareEth->ETH_pMacRegs = (__PETH_MAC_REGS)pDesignwareEth->ETH_ulBase;     /*  MAC 寄存器地址  */
    pDesignwareEth->ETH_pDmaRegs = (__PETH_DMA_REGS)(pDesignwareEth->ETH_ulBase + \
                                    DW_DMA_BASE_OFFSET);                /*  DMA 寄存器地址              */

    pDesignwareEth->ETH_pTxDmaDescrTable = API_VmmDmaAllocAlign(
                                           sizeof(__DMA_MAC_DESCR) * CONFIG_TX_DESCR_NUM,
                                           ARCH_DMA_MINALIGN);          /*  分配发送 DMA 描述符表       */
    if (pDesignwareEth->ETH_pTxDmaDescrTable == LW_NULL) {
        printk(KERN_ERR "designwareEthInit(): failed to alloc tx descrs!\n");
        return  (ERR_MEM);
    }

    pDesignwareEth->ETH_pRxDmaDescrTable = API_VmmDmaAllocAlign(
                                           sizeof(__DMA_MAC_DESCR) * CONFIG_RX_DESCR_NUM,
                                           ARCH_DMA_MINALIGN);          /*  分配接收 DMA 描述符表       */
    if (pDesignwareEth->ETH_pRxDmaDescrTable == LW_NULL) {
        printk(KERN_ERR "designwareEthInit(): failed to alloc rx descrs!\n");
        return  (ERR_MEM);
    }

    pDesignwareEth->ETH_pucTxDmaBuffers = API_VmmDmaAllocAlign(
                                          TX_TOTAL_BUFSIZE,
                                          ARCH_DMA_MINALIGN);           /*  分配发送 DMA 缓冲区         */
    if (pDesignwareEth->ETH_pucTxDmaBuffers == LW_NULL) {
        printk(KERN_ERR "designwareEthInit(): failed to alloc tx buffers!\n");
        return  (ERR_MEM);
    }

    pDesignwareEth->ETH_pucRxDmaBuffers = API_VmmDmaAllocAlign(
                                          RX_TOTAL_BUFSIZE,
                                          ARCH_DMA_MINALIGN);           /*  分配接收 DMA 缓冲区         */
    if (pDesignwareEth->ETH_pucRxDmaBuffers == LW_NULL) {
        printk(KERN_ERR "designwareEthInit(): failed to alloc rx buffers!\n");
        return  (ERR_MEM);
    }

    pDesignwareEth->ETH_hTxDmaDescrSem = API_SemaphoreCCreate("ethif_tx_sem",
                                                              CONFIG_TX_DESCR_NUM,
                                                              CONFIG_TX_DESCR_NUM,
                                                              LW_OPTION_WAIT_FIFO |
                                                              LW_OPTION_OBJECT_GLOBAL,
                                                              LW_NULL); /*  创建发送信号量              */
    if (pDesignwareEth->ETH_hTxDmaDescrSem == LW_OBJECT_HANDLE_INVALID) {
        printk(KERN_ERR "designwareEthInit(): failed to create tx semaphore!\n");
        return  (ERR_MEM);
    }

    LW_SPIN_INIT(&pDesignwareEth->ETH_spinlock);                        /*  初始化自旋锁                */

    API_InterVectorDisable(pDesignwareEth->ETH_platfromData.ulVector);  /*  关闭中断                    */

    iError = __designwareEthInit(pNetif);                               /*  初始化网卡                  */
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "designwareEthInit(): failed to init designware ethernet controlor!\n");
        return  (ERR_IF);
    }

    API_InterVectorConnect((ULONG)pDesignwareEth->ETH_platfromData.ulVector,
                           (PINT_SVR_ROUTINE)__designwareEthIsr,
                           (PVOID)pNetif,
                           "ethif_isr");                                /*  连接中断服务函数            */

    API_InterVectorEnable(pDesignwareEth->ETH_platfromData.ulVector);   /*  使能中断                    */

    return  (ERR_OK);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
