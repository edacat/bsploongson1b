/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: config.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 12 �� 09 ��
**
** ��        ��: ����������.
*********************************************************************************************************/
#include "../../config.h"
/*********************************************************************************************************
  Chip
*********************************************************************************************************/
#define LS1BSOC
#define BSP_CFG_BOOTLOAD_SIZE           (2   * 1024 * 1024)
#define BSP_CFG_CPU_FREQ                (220 * 1000 * 1000)
/*********************************************************************************************************
  Interrupt
*********************************************************************************************************/
#define BSP_CFG_INT0_BASE               (0xBFD01040)
#define BSP_CFG_INT1_BASE               (0xBFD01058)
#define BSP_CFG_INT2_BASE               (0xBFD01070)
#define BSP_CFG_INT3_BASE               (0xBFD01088)

#define BSP_CFG_INT0_VECTOR             (2)
#define BSP_CFG_INT1_VECTOR             (3)
#define BSP_CFG_INT2_VECTOR             (4)
#define BSP_CFG_INT3_VECTOR             (5)

#define BSP_CFG_TIMER_VECTOR            (7)

#define BSP_CFG_INT_SUB_VECTOR(i, vector)  (8 + 32 * (i) + (vector))

#define BSP_CFG_INT0_SUB_VECTOR(vector) BSP_CFG_INT_SUB_VECTOR(0, vector)
#define BSP_CFG_INT1_SUB_VECTOR(vector) BSP_CFG_INT_SUB_VECTOR(1, vector)
#define BSP_CFG_INT2_SUB_VECTOR(vector) BSP_CFG_INT_SUB_VECTOR(2, vector)
#define BSP_CFG_INT3_SUB_VECTOR(vector) BSP_CFG_INT_SUB_VECTOR(3, vector)
/*********************************************************************************************************
  16C550 UART
*********************************************************************************************************/
#define BSP_CFG_16C550_SIO_NR           (5)

#define BSP_CFG_16C550_UART0_BASE       (0xBFE40000)
#define BSP_CFG_16C550_UART1_BASE       (0xBFE44000)
#define BSP_CFG_16C550_UART2_BASE       (0xBFE48000)
#define BSP_CFG_16C550_UART3_BASE       (0xBFE4C000)
#define BSP_CFG_16C550_UART4_BASE       (0xBFE6C000)
#define BSP_CFG_16C550_UART5_BASE       (0xBFE7C000)

#define BSP_CFG_16C550_UART0_VECTOR     BSP_CFG_INT0_SUB_VECTOR(2)
#define BSP_CFG_16C550_UART1_VECTOR     BSP_CFG_INT0_SUB_VECTOR(3)
#define BSP_CFG_16C550_UART2_VECTOR     BSP_CFG_INT0_SUB_VECTOR(4)
#define BSP_CFG_16C550_UART3_VECTOR     BSP_CFG_INT0_SUB_VECTOR(5)
#define BSP_CFG_16C550_UART4_VECTOR     BSP_CFG_INT0_SUB_VECTOR(29)
#define BSP_CFG_16C550_UART5_VECTOR     BSP_CFG_INT0_SUB_VECTOR(30)

#define BSP_CFG_16C550_BAUD             (115200)

#define BSP_CFG_16C550_XTAL             (230400)
/*********************************************************************************************************
  GMAC
*********************************************************************************************************/
#define BSP_CFG_GMAC0_BASE              (0xBFE10000)
#define BSP_CFG_GMAC1_BASE              (0xBFE20000)
#define BSP_CFG_GMAC0_PHY_ID            (0x8000)
#define BSP_CFG_GMAC1_PHY_ID            (0x8000)
/*********************************************************************************************************
  WDT
*********************************************************************************************************/
#define BSP_CFG_WDT_BASE                (0xBFE5C060)
#define BSP_CFG_WDT_EN                  (0x00)
#define BSP_CFG_WDT_TIMER               (0x04)
#define BSP_CFG_WDT_SET                 (0x08)
/*********************************************************************************************************
  END
*********************************************************************************************************/
