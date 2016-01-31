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
** ��   ��   ��: ls1b_16c550_sio.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 09 ��
**
** ��        ��: Loongson1b ƽ̨ 16C550 SIO ���������ļ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include <SylixOS.h>
#include <driver/sio/16c550.h>
#include "16c550_sio_priv.h"
/*********************************************************************************************************
  Loongson1b ƽ̨ 16C550 SIO ���ó�ʼ��
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
