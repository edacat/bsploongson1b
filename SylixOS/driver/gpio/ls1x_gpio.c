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
** 文   件   名: ls1x_gpio.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 12 月 10 日
**
** 描        述: LS1X GPIO 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"                                                    /*  操作系统                    */
#include <linux/compat.h>

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define LS1X_GPIO0_BASE_ADDR    (0xBFD010C0)
#define LS1X_GPIO1_BASE_ADDR    (0xBFD010C4)
#define LS1X_GPIO2_BASE_ADDR    (0xBFD010C8)
#define LS1X_GPIO3_BASE_ADDR    (0xBFD010CC)

#define LS1X_GPIO_CFG_OF        (0)                                     /*  配置寄存器                  */
#define LS1X_GPIO_OE_OF         (0x10)                                  /*  输入使能寄存器              */
#define LS1X_GPIO_IN_OF         (0x20)                                  /*  输入寄存器                  */
#define LS1X_GPIO_OUT_OF        (0x30)                                  /*  输出寄存器                  */

#define LS1X_GPIO_NUM           (64)

#ifdef LS1ASOC
/* GPIO 64-87 group  2 */
#define LS1X_GPIO_NUM           (88)
#endif

#ifdef LS1CSOC
/* GPIO 64-95 group  2 */
/* GPIO 96-127 group 3 */
#define LS1X_GPIO_NUM           (128)
#endif

#define PIN_BITS                (5)
#define PINS_PER_BANK           (1 << PIN_BITS)
#define PINID_TO_BANK(id)       ((id) >> PIN_BITS)
#define PINID_TO_PIN(id)        ((id) & (PINS_PER_BANK - 1))
#define PINID_MASK(id)          (1 << (id))

#define HIGH                    (1)
#define LOW                     (0)
#define OUT                     (1)
#define IN                      (0)

/*********************************************************************************************************
  GPIO控制器基地址
*********************************************************************************************************/
static addr_t   _G_GPIOBase[] = {
        LS1X_GPIO0_BASE_ADDR,
        LS1X_GPIO1_BASE_ADDR,
        LS1X_GPIO2_BASE_ADDR,
        LS1X_GPIO3_BASE_ADDR,
};
/*********************************************************************************************************
** 函数名称: ls1xGpioGetDirection
** 功能描述: 获得指定 GPIO 方向
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 输入 1:输出
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xGpioGetDirection (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT    uiBank;
    UINT32  uiRegOe;
    UINT32  uiTemp;

    uiBank   = PINID_TO_BANK(uiOffset);
    uiRegOe  = _G_GPIOBase[uiBank] + LS1X_GPIO_OE_OF;

    uiTemp   = readl(uiRegOe);

    uiTemp   = ((uiTemp >> PINID_TO_PIN(uiOffset)) & OUT);

    return (uiTemp);
}
/*********************************************************************************************************
** 函数名称: ls1xGpioGet
** 功能描述: 获得指定 GPIO 电平
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 低电平 1:高电平
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xGpioGet (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT    uiBank;
    UINT32  uiRegIn;
    UINT32  uiTemp;

    uiBank   = PINID_TO_BANK(uiOffset);
    uiRegIn  = _G_GPIOBase[uiBank] + LS1X_GPIO_IN_OF;

    uiTemp   = readl(uiRegIn);

    uiTemp   = ((uiTemp >> PINID_TO_PIN(uiOffset)) & HIGH);

    return (uiTemp);
}
/*********************************************************************************************************
** 函数名称: ls1xGpioSet
** 功能描述: 设置指定 GPIO 电平
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iValue      输出电平
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls1xGpioSet (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    UINT    uiBank;
    UINT32  uiRegOut;
    UINT32  uiTemp;

    uiBank   = PINID_TO_BANK(uiOffset);
    uiRegOut = _G_GPIOBase[uiBank] + LS1X_GPIO_OUT_OF;

    uiTemp   = readl(uiRegOut);
    if (iValue) {
        uiTemp |= PINID_MASK(PINID_TO_PIN(uiOffset));
    } else {
        uiTemp &= ~(PINID_MASK(PINID_TO_PIN(uiOffset)));
    }

    writel(uiTemp, uiRegOut);
}
/*********************************************************************************************************
** 函数名称: ls1xGpioDirectionInput
** 功能描述: 设置指定 GPIO 为输入模式
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xGpioDirectionInput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT    uiBank;
    UINT32  uiRegCfg;
    UINT32  uiRegOe;
    UINT32  uiTemp;

    uiBank   = PINID_TO_BANK(uiOffset);

    uiRegCfg = _G_GPIOBase[uiBank] + LS1X_GPIO_CFG_OF;
    uiRegOe  = _G_GPIOBase[uiBank] + LS1X_GPIO_OE_OF;

    uiTemp   = readl(uiRegCfg);
    uiTemp  |= PINID_MASK(PINID_TO_PIN(uiOffset));
    writel(uiTemp, uiRegCfg);
    uiTemp   = readl(uiRegOe);
    uiTemp  |= PINID_MASK(PINID_TO_PIN(uiOffset));
    writel(uiTemp, uiRegOe);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xGpioDirectionOutput
** 功能描述: 设置指定 GPIO 为输出模式
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           iValue      输出电平
** 输  出  : 0: 正确 -1:错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xGpioDirectionOutput (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, INT  iValue)
{
    UINT    uiBank;
    UINT32  uiRegCfg;
    UINT32  uiRegOe;
    UINT32  uiTemp;

    ls1xGpioSet(pGpioChip, uiOffset, iValue);

    uiBank = PINID_TO_BANK(uiOffset);

    uiRegCfg = _G_GPIOBase[uiBank] + LS1X_GPIO_CFG_OF;
    uiRegOe  = _G_GPIOBase[uiBank] + LS1X_GPIO_OE_OF;

    uiTemp = readl(uiRegCfg);
    uiTemp |= PINID_MASK(PINID_TO_PIN(uiOffset));
    writel(uiTemp, uiRegCfg);
    uiTemp = readl(uiRegOe);
    uiTemp &= ~(PINID_MASK(PINID_TO_PIN(uiOffset)));
    writel(uiTemp, uiRegOe);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xGpioSetupIrq
** 功能描述: 设置指定 GPIO 为外部中断输入管脚
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
**           bIsLevel    是否为电平触发
**           uiType      如果为电平触发, 1 表示高电平触发, 0 表示低电平触发
**                       如果为边沿触发, 1 表示上升沿触发, 0 表示下降沿触发, 2 双边沿触发
** 输  出  : IRQ 向量号 -1:错误
** 全局变量:
** 调用模块:
** INT2: GPIO0-GPIO31, INT3: GPIO32-GPIO63
*********************************************************************************************************/
static INT  ls1xGpioSetupIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset, BOOL  bIsLevel, UINT  uiType)
{
    UINT    uiBank;
    UINT    uiPinID;
    ULONG   ulVector;

    uiBank  = PINID_TO_BANK(uiOffset);
    uiPinID = PINID_TO_PIN(uiOffset);

    /*
     * 计算对应的向量号
     */
    ulVector = BSP_CFG_INT_SUB_VECTOR((uiBank + 2), uiPinID);

    if (bIsLevel) {
        /*
         *  电平触发
         */
        writel(0xFFFFFFFF, uiBank == 0 ? BSP_CFG_INT2_BASE + 0x10: BSP_CFG_INT3_BASE + 0x10);

    } else {
        /*
         *  边沿触发
         */
        writel(0x0000E000, uiBank == 0 ? BSP_CFG_INT2_BASE + 0x1C: BSP_CFG_INT3_BASE + 0x1C);
    }

    return  (ulVector);
}
/*********************************************************************************************************
** 函数名称: ls1xGpioClearIrq
** 功能描述: 清除指定 GPIO 中断标志
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls1xGpioClearIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT    uiBank;
    UINT    uiPinID;
    ULONG   ulVector;

    uiBank  = PINID_TO_BANK(uiOffset);
    uiPinID = PINID_TO_PIN(uiOffset);

    /*
     * 计算对应的向量号
     */
    ulVector = BSP_CFG_INT_SUB_VECTOR((uiBank + 2), uiPinID);

    API_InterVectorDisable(ulVector);
}
/*********************************************************************************************************
** 函数名称: ls1xGpioSvrIrq
** 功能描述: 判断 GPIO 中断标志
** 输  入  : pGpioChip   GPIO 芯片
**           uiOffset    GPIO 针对 BASE 的偏移量
** 输  出  : 中断返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  ls1xGpioSvrIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT    uiBank;
    UINT    uiPinID;
    ULONG   ulVector;

    uiBank  = PINID_TO_BANK(uiOffset);
    uiPinID = PINID_TO_PIN(uiOffset);

    /*
     * 计算对应的向量号
     */
    ulVector = BSP_CFG_INT_SUB_VECTOR((uiBank + 2), uiPinID);

    if (bspIntVectorIsEnable(ulVector)) {
        return  (LW_IRQ_HANDLED);
    } else {
        return (LW_IRQ_NONE);
    }
}
/*********************************************************************************************************
  GPIO 驱动程序
*********************************************************************************************************/
static LW_GPIO_CHIP  _G_ls1xGpioChip = {
        .GC_pcLabel              = "LS1X GPIO",

        .GC_pfuncRequest         = LW_NULL,
        .GC_pfuncFree            = LW_NULL,

        .GC_pfuncGetDirection    = ls1xGpioGetDirection,
        .GC_pfuncDirectionInput  = ls1xGpioDirectionInput,
        .GC_pfuncGet             = ls1xGpioGet,
        .GC_pfuncDirectionOutput = ls1xGpioDirectionOutput,
        .GC_pfuncSetDebounce     = LW_NULL,
        .GC_pfuncSetPull         = LW_NULL,
        .GC_pfuncSet             = ls1xGpioSet,
        .GC_pfuncSetupIrq        = ls1xGpioSetupIrq,
        .GC_pfuncClearIrq        = ls1xGpioClearIrq,
        .GC_pfuncSvrIrq          = ls1xGpioSvrIrq,

        .GC_uiBase               = 0,
        .GC_uiNGpios             = LS1X_GPIO_NUM,
};
/*********************************************************************************************************
** 函数名称: ls1xGpioDrv
** 功能描述: 创建 ls1x GPIO 驱动
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ls1xGpioDrv (VOID)
{
    gpioChipAdd(&_G_ls1xGpioChip);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

