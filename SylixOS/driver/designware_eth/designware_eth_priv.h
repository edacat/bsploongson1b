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
** 文   件   名: designware_eth_priv.h
**
** 创   建   人: Jiao.JinXing
**
** 文件创建日期: 2015 年 10 月 29 日
**
** 描        述: designware 以太网芯片驱动私有头文件.
*********************************************************************************************************/
#ifndef DESIGNWARE_ETH_PRIV_H_
#define DESIGNWARE_ETH_PRIV_H_
/*********************************************************************************************************
  MAC configuration register definitions
*********************************************************************************************************/
#define FRAMEBURSTENABLE            (1 << 21)
#define MII_PORTSELECT              (1 << 15)
#define FES_100                     (1 << 14)
#define DISABLERXOWN                (1 << 13)
#define FULLDPLXMODE                (1 << 11)
#define RXENABLE                    (1 << 2)
#define TXENABLE                    (1 << 3)
/*********************************************************************************************************
  DMA Interrupt Enable register defines
*********************************************************************************************************/
/* NORMAL INTERRUPT */
#define DMA_INTR_ENA_NIE            0x00010000                          /*  Normal Summary              */
#define DMA_INTR_ENA_TIE            0x00000001                          /*  Transmit Interrupt          */
#define DMA_INTR_ENA_TUE            0x00000004                          /*  Transmit Buffer Unavailable */
#define DMA_INTR_ENA_RIE            0x00000040                          /*  Receive Interrupt           */
#define DMA_INTR_ENA_ERE            0x00004000                          /*  Early Receive               */

#define DMA_INTR_NORMAL             (DMA_INTR_ENA_NIE | DMA_INTR_ENA_RIE | \
                                    DMA_INTR_ENA_TIE | DMA_INTR_ENA_TUE)

/* ABNORMAL INTERRUPT */
#define DMA_INTR_ENA_AIE            0x00008000                          /*  Abnormal Summary            */
#define DMA_INTR_ENA_FBE            0x00002000                          /*  Fatal Bus Error             */
#define DMA_INTR_ENA_ETE            0x00000400                          /*  Early Transmit              */
#define DMA_INTR_ENA_RWE            0x00000200                          /*  Receive Watchdog            */
#define DMA_INTR_ENA_RSE            0x00000100                          /*  Receive Stopped             */
#define DMA_INTR_ENA_RUE            0x00000080                          /*  Receive Buffer Unavailable  */
#define DMA_INTR_ENA_UNE            0x00000020                          /*  Tx Underflow                */
#define DMA_INTR_ENA_OVE            0x00000010                          /*  Receive Overflow            */
#define DMA_INTR_ENA_TJE            0x00000008                          /*  Transmit Jabber             */
#define DMA_INTR_ENA_TSE            0x00000002                          /*  Transmit Stopped            */

#define DMA_INTR_ABNORMAL           (DMA_INTR_ENA_AIE | DMA_INTR_ENA_FBE | \
                                    DMA_INTR_ENA_UNE | DMA_INTR_ENA_RUE)

/* DMA default interrupt mask */
#define DMA_INTR_DEFAULT_MASK       (DMA_INTR_NORMAL | DMA_INTR_ABNORMAL)
/*********************************************************************************************************
  DMA Status register defines
*********************************************************************************************************/
#define DMA_STATUS_GPI              0x10000000                          /*  PMT interrupt               */
#define DMA_STATUS_GMI              0x08000000                          /*  MMC interrupt               */
#define DMA_STATUS_GLI              0x04000000                          /*  GMAC Line interface int.    */
#define DMA_STATUS_GMI              0x08000000
#define DMA_STATUS_GLI              0x04000000
#define DMA_STATUS_EB_MASK          0x00380000                          /*  Error Bits Mask             */
#define DMA_STATUS_EB_TX_ABORT      0x00080000                          /*  Error Bits - TX Abort       */
#define DMA_STATUS_EB_RX_ABORT      0x00100000                          /*  Error Bits - RX Abort       */
#define DMA_STATUS_TS_MASK          0x00700000                          /*  Transmit Process State      */
#define DMA_STATUS_TS_SHIFT         20
#define DMA_STATUS_RS_MASK          0x000e0000                          /*  Receive Process State       */
#define DMA_STATUS_RS_SHIFT         17
#define DMA_STATUS_NIS              0x00010000                          /*  Normal Interrupt Summary    */
#define DMA_STATUS_AIS              0x00008000                          /*  Abnormal Interrupt Summary  */
#define DMA_STATUS_ERI              0x00004000                          /*  Early Receive Interrupt     */
#define DMA_STATUS_FBI              0x00002000                          /*  Fatal Bus Error Interrupt   */
#define DMA_STATUS_ETI              0x00000400                          /*  Early Transmit Interrupt    */
#define DMA_STATUS_RWT              0x00000200                          /*  Receive Watchdog Timeout    */
#define DMA_STATUS_RPS              0x00000100                          /*  Receive Process Stopped     */
#define DMA_STATUS_RU               0x00000080                          /*  Receive Buffer Unavailable  */
#define DMA_STATUS_RI               0x00000040                          /*  Receive Interrupt           */
#define DMA_STATUS_UNF              0x00000020                          /*  Transmit Underflow          */
#define DMA_STATUS_OVF              0x00000010                          /*  Receive Overflow            */
#define DMA_STATUS_TJT              0x00000008                          /*  Transmit Jabber Timeout     */
#define DMA_STATUS_TU               0x00000004                          /*  Transmit Buffer Unavailable */
#define DMA_STATUS_TPS              0x00000002                          /*  Transmit Process Stopped    */
#define DMA_STATUS_TI               0x00000001                          /*  Transmit Interrupt          */
/*********************************************************************************************************
  MII address register definitions
*********************************************************************************************************/
#define __MII_BUSY                  (1 << 0)
#define __MII_WRITE                 (1 << 1)
#define MII_CLKRANGE_60_100M        (0)
#define MII_CLKRANGE_100_150M       (0x4)
#define MII_CLKRANGE_20_35M         (0x8)
#define MII_CLKRANGE_35_60M         (0xC)
#define MII_CLKRANGE_150_250M       (0x10)
#define MII_CLKRANGE_250_300M       (0x14)

#define MIIADDRSHIFT                (11)
#define MIIREGSHIFT                 (6)
#define MII_REGMSK                  (0x1F << 6)
#define MII_ADDRMSK                 (0x1F << 11)

#define DW_DMA_BASE_OFFSET          (0x1000)
/*********************************************************************************************************
  Default DMA Burst length
*********************************************************************************************************/
#ifndef CONFIG_DW_GMAC_DEFAULT_DMA_PBL
#define CONFIG_DW_GMAC_DEFAULT_DMA_PBL 8
#endif
/*********************************************************************************************************
  Bus mode register definitions
*********************************************************************************************************/
#define FIXEDBURST                  (1 << 16)
#define PRIORXTX_41                 (3 << 14)
#define PRIORXTX_31                 (2 << 14)
#define PRIORXTX_21                 (1 << 14)
#define PRIORXTX_11                 (0 << 14)
#define DMA_PBL                     (CONFIG_DW_GMAC_DEFAULT_DMA_PBL << 8)
#define RXHIGHPRIO                  (1 << 1)
#define DMAMAC_SRST                 (1 << 0)
/*********************************************************************************************************
  Poll demand definitions
*********************************************************************************************************/
#define POLL_DATA                   (0xFFFFFFFF)
/*********************************************************************************************************
  Operation mode definitions
*********************************************************************************************************/
#define STOREFORWARD                (1 << 21)
#define FLUSHTXFIFO                 (1 << 20)
#define TXSTART                     (1 << 13)
#define TXSECONDFRAME               (1 << 2)
#define RXSTART                     (1 << 1)
/*********************************************************************************************************
  Descriptior related definitions
*********************************************************************************************************/
#define MAC_MAX_FRAME_SZ            (1600)
/*********************************************************************************************************
  txrx_status definitions
*********************************************************************************************************/
/*********************************************************************************************************
  tx status bits definitions
*********************************************************************************************************/
#if defined(CONFIG_DW_ALTDESCRIPTOR)

#define DESC_TXSTS_OWNBYDMA         (1 << 31)
#define DESC_TXSTS_TXINT            (1 << 30)
#define DESC_TXSTS_TXLAST           (1 << 29)
#define DESC_TXSTS_TXFIRST          (1 << 28)
#define DESC_TXSTS_TXCRCDIS         (1 << 27)

#define DESC_TXSTS_TXPADDIS         (1 << 26)
#define DESC_TXSTS_TXCHECKINSCTRL   (3 << 22)
#define DESC_TXSTS_TXRINGEND        (1 << 21)
#define DESC_TXSTS_TXCHAIN          (1 << 20)
#define DESC_TXSTS_MSK              (0x1FFFF << 0)

#else

#define DESC_TXSTS_OWNBYDMA         (1 << 31)
#define DESC_TXSTS_MSK              (0x1FFFF << 0)

#endif
/*********************************************************************************************************
  rx status bits definitions
*********************************************************************************************************/
#define DESC_RXSTS_OWNBYDMA         (1 << 31)
#define DESC_RXSTS_DAFILTERFAIL     (1 << 30)
#define DESC_RXSTS_FRMLENMSK        (0x3FFF << 16)
#define DESC_RXSTS_FRMLENSHFT       (16)

#define DESC_RXSTS_ERROR            (1 << 15)
#define DESC_RXSTS_RXTRUNCATED      (1 << 14)
#define DESC_RXSTS_SAFILTERFAIL     (1 << 13)
#define DESC_RXSTS_RXIPC_GIANTFRAME (1 << 12)
#define DESC_RXSTS_RXDAMAGED        (1 << 11)
#define DESC_RXSTS_RXVLANTAG        (1 << 10)
#define DESC_RXSTS_RXFIRST          (1 << 9)
#define DESC_RXSTS_RXLAST           (1 << 8)
#define DESC_RXSTS_RXIPC_GIANT      (1 << 7)
#define DESC_RXSTS_RXCOLLISION      (1 << 6)
#define DESC_RXSTS_RXFRAMEETHER     (1 << 5)
#define DESC_RXSTS_RXWATCHDOG       (1 << 4)
#define DESC_RXSTS_RXMIIERROR       (1 << 3)
#define DESC_RXSTS_RXDRIBBLING      (1 << 2)
#define DESC_RXSTS_RXCRC            (1 << 1)
/*********************************************************************************************************
  dmamac_cntl definitions
*********************************************************************************************************/
/*********************************************************************************************************
  tx control bits definitions
*********************************************************************************************************/
#if defined(CONFIG_DW_ALTDESCRIPTOR)

#define DESC_TXCTRL_SIZE1MASK       (0x1FFF << 0)
#define DESC_TXCTRL_SIZE1SHFT       (0)
#define DESC_TXCTRL_SIZE2MASK       (0x1FFF << 16)
#define DESC_TXCTRL_SIZE2SHFT       (16)

#else

#define DESC_TXCTRL_TXINT           (1 << 31)
#define DESC_TXCTRL_TXLAST          (1 << 30)
#define DESC_TXCTRL_TXFIRST         (1 << 29)
#define DESC_TXCTRL_TXCHECKINSCTRL  (3 << 27)
#define DESC_TXCTRL_TXCRCDIS        (1 << 26)
#define DESC_TXCTRL_TXRINGEND       (1 << 25)
#define DESC_TXCTRL_TXCHAIN         (1 << 24)

#define DESC_TXCTRL_SIZE1MASK       (0x7FF << 0)
#define DESC_TXCTRL_SIZE1SHFT       (0)
#define DESC_TXCTRL_SIZE2MASK       (0x7FF << 11)
#define DESC_TXCTRL_SIZE2SHFT       (11)

#endif
/*********************************************************************************************************
  rx control bits definitions
*********************************************************************************************************/
#if defined(CONFIG_DW_ALTDESCRIPTOR)

#define DESC_RXCTRL_RXINTDIS        (1 << 31)
#define DESC_RXCTRL_RXRINGEND       (1 << 15)
#define DESC_RXCTRL_RXCHAIN         (1 << 14)

#define DESC_RXCTRL_SIZE1MASK       (0x1FFF << 0)
#define DESC_RXCTRL_SIZE1SHFT       (0)
#define DESC_RXCTRL_SIZE2MASK       (0x1FFF << 16)
#define DESC_RXCTRL_SIZE2SHFT       (16)

#else

#define DESC_RXCTRL_RXINTDIS        (1 << 31)
#define DESC_RXCTRL_RXRINGEND       (1 << 25)
#define DESC_RXCTRL_RXCHAIN         (1 << 24)

#define DESC_RXCTRL_SIZE1MASK       (0x7FF << 0)
#define DESC_RXCTRL_SIZE1SHFT       (0)
#define DESC_RXCTRL_SIZE2MASK       (0x7FF << 11)
#define DESC_RXCTRL_SIZE2SHFT       (11)

#endif

#endif                                                                  /*  DESIGNWARE_ETH_PRIV_H_      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
