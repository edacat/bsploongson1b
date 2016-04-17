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
** 文   件   名: ls1b_int.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 11 月 30 日
**
** 描        述: Loongson1b 处理器中断支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
/*********************************************************************************************************
  中断控制器基地址
*********************************************************************************************************/
static addr_t   _G_aulIntCtrlBase[4] = {
        BSP_CFG_INT0_BASE,
        BSP_CFG_INT1_BASE,
        BSP_CFG_INT2_BASE,
        BSP_CFG_INT3_BASE,
};
/*********************************************************************************************************
  寄存器偏移
*********************************************************************************************************/
#define INTISR                  (0x00)                                  /*  中断控制状态寄存器          */
#define INTIEN                  (0x04)                                  /*  中断控制使能寄存器          */
#define INTSET                  (0x08)                                  /*  中断置位寄存器              */
#define INTCLR                  (0x0C)                                  /*  中断清空寄存器              */
#define INTPOL                  (0x10)                                  /*  高电平触发中断使能寄存器    */
#define INTEDGE                 (0x14)                                  /*  边沿触发中断使能寄存器      */
/*********************************************************************************************************
** 函数名称: ls1bIntInit
** 功能描述: 中断系统初始化
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ls1bIntInit (VOID)
{
    UINT32  uiIntCtrlIndex;

    for (uiIntCtrlIndex = 0; uiIntCtrlIndex < 4; uiIntCtrlIndex++) {
        write32(0xffffffff, _G_aulIntCtrlBase[uiIntCtrlIndex] + INTPOL);
        write32(0x0000e000, _G_aulIntCtrlBase[uiIntCtrlIndex] + INTEDGE);
        write32(0xffffffff, _G_aulIntCtrlBase[uiIntCtrlIndex] + INTCLR);
    }
}
/*********************************************************************************************************
** 函数名称: ls1bIntHandle
** 功能描述: 中断入口
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ls1bIntHandle (UINT32  uiIntCtrlIndex, BOOL  bPreemptive)
{
    addr_t  ulIntCtrlBase;
    UINT32  uiValue;
    UINT32  uiSubVector;

    ulIntCtrlBase  = _G_aulIntCtrlBase[uiIntCtrlIndex];
    uiValue        = read32(ulIntCtrlBase + INTISR);

    for (uiSubVector = 0; uiSubVector < 32; uiSubVector++) {
        if (uiValue & (1 << uiSubVector)) {
            archIntHandle(BSP_CFG_INT_SUB_VECTOR(uiIntCtrlIndex, uiSubVector),
                          bPreemptive);

            write32((1 << uiSubVector) | read32(ulIntCtrlBase + INTCLR),
                    ulIntCtrlBase + INTCLR);
        }
    }
}
/*********************************************************************************************************
** 函数名称: ls1bIntVectorEnable
** 功能描述: 使能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ls1bIntVectorEnable (ULONG  ulVector)
{
    UINT32  uiValue;
    addr_t  ulIntCtrlBase;

    ulIntCtrlBase = _G_aulIntCtrlBase[ulVector / 32];

    uiValue  = read32(ulIntCtrlBase + INTIEN);
    uiValue |= 1 << (ulVector % 32);
    write32(uiValue, ulIntCtrlBase + INTIEN);
}
/*********************************************************************************************************
** 函数名称: ls1bIntVectorDisable
** 功能描述: 禁能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ls1bIntVectorDisable (ULONG  ulVector)
{
    UINT32  uiValue;
    addr_t  ulIntCtrlBase;

    ulIntCtrlBase = _G_aulIntCtrlBase[ulVector / 32];

    uiValue  = read32(ulIntCtrlBase + INTIEN);
    uiValue &= ~(1 << (ulVector % 32));
    write32(uiValue, ulIntCtrlBase + INTIEN);
}
/*********************************************************************************************************
** 函数名称: ls1bIntVectorIsEnable
** 功能描述: 检查指定的中断向量是否使能
** 输  入  : ulVector     中断向量
** 输  出  : LW_FALSE 或 LW_TRUE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  ls1bIntVectorIsEnable (ULONG  ulVector)
{
    UINT32  uiValue;
    addr_t  ulIntCtrlBase;

    ulIntCtrlBase = _G_aulIntCtrlBase[ulVector / 32];

    uiValue  = read32(ulIntCtrlBase + INTIEN);
    if (uiValue & (1 << (ulVector % 32))) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
