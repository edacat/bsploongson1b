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
** 文   件   名: ls1b_16c550_sio.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 09 月 09 日
**
** 描        述: Loongson1b 平台 16C550 SIO 驱动配置文件
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include <SylixOS.h>
#include <driver/sio/16c550.h>
#include "16c550_sio_priv.h"
/*********************************************************************************************************
  Loongson1b 平台 16C550 SIO 配置初始化
*********************************************************************************************************/
SIO16C550_CFG      _G_sio16C550Cfgs[BSP_CFG_16C550_SIO_NR] = {
        {
                BSP_CFG_16C550_UART0_BASE,
                BSP_CFG_16C550_XTAL,
                BSP_CFG_16C550_BAUD,
                BSP_CFG_16C550_UART0_VECTOR,
        },
        {
                BSP_CFG_16C550_UART1_BASE,
                BSP_CFG_16C550_XTAL,
                BSP_CFG_16C550_BAUD,
                BSP_CFG_16C550_UART1_VECTOR,
        },
        {
                BSP_CFG_16C550_UART2_BASE,
                BSP_CFG_16C550_XTAL,
                BSP_CFG_16C550_BAUD,
                BSP_CFG_16C550_UART2_VECTOR,
        },
        {
                BSP_CFG_16C550_UART3_BASE,
                BSP_CFG_16C550_XTAL,
                BSP_CFG_16C550_BAUD,
                BSP_CFG_16C550_UART3_VECTOR,
        },
        {
                BSP_CFG_16C550_UART4_BASE,
                BSP_CFG_16C550_XTAL,
                BSP_CFG_16C550_BAUD,
                BSP_CFG_16C550_UART4_VECTOR,
        },
#if 0
        {
                BSP_CFG_16C550_UART5_BASE,
                BSP_CFG_16C550_XTAL,
                BSP_CFG_16C550_BAUD,
                BSP_CFG_16C550_UART5_VECTOR,
        },
#endif
};
/*********************************************************************************************************
  END
*********************************************************************************************************/
