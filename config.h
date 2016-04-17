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
** ��   ��   ��: RealEvo-IDE
**
** �ļ���������: 2015 ��11 ��30 ��
**
** ��        ��: ������RealEvo-IDE���ɣ���������Makefile���ܣ������ֶ��޸�
*********************************************************************************************************/
#ifndef __CONFIG_H
#define __CONFIG_H

#define MIPS32_KSEG0_PA(va)             ((va) & (~(0x1 << 31)))
#define MIPS32_KSEG1_PA(va)             ((va) & (~(0x7 << 29)))

#define BSP_CFG_ROM_BASE                0xBFC00000
#define BSP_CFG_ROM_PA_BASE             MIPS32_KSEG1_PA(BSP_CFG_ROM_BASE)
#define BSP_CFG_ROM_SIZE                0x00C00000

#define BSP_CFG_RAM_BASE                0x80200000
#define BSP_CFG_RAM_PA_BASE             MIPS32_KSEG0_PA(BSP_CFG_RAM_BASE)
#define BSP_CFG_RAM_SIZE                (47 * 1024 * 1024)

#define BSP_CFG_KERNEL_SIZE             (6 * 1024 * 1024)

#define BSP_CFG_DATA_BASE               (BSP_CFG_RAM_BASE + BSP_CFG_KERNEL_SIZE)
#define BSP_CFG_DATA_PA_BASE            MIPS32_KSEG0_PA(BSP_CFG_DATA_BASE)

#define BSP_CFG_COMMON_MEM_SIZE         (24 * 1024 * 1024)

#define BSP_CFG_SYS_INIT_SP_ADDR        (BSP_CFG_RAM_BASE + BSP_CFG_COMMON_MEM_SIZE)

#endif                                                                  /*  __CONFIG_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
