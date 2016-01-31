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
** 文   件   名: ls1x_clock.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 12 月 09 日
**
** 描        述: loongson1B Clock 驱动源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include <linux/compat.h>
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#if defined(LS1ASOC)
#define AHB_CLK                 (33*1000*1000)
#elif defined(LS1BSOC)
#define AHB_CLK                 (33*1000*1000)
#elif defined(LS1BSOC_CORE)
#define AHB_CLK                 (25*1000*1000)
#elif defined(LS1CSOC)
#define AHB_CLK                 (24*1000*1000)
#endif

#define APB_CLK                 AHB_CLK
#define LS1X_CLK_PLL_FREQ       (0xBFE78030)
#define LS1X_CLK_PLL_DIV        (0xBFE78034)

#define DIV_DC_EN               (0x01 << 31)
#define DIV_DC                  (0x1F << 26)
#define DIV_CPU_EN              (0x01 << 25)
#define DIV_CPU                 (0x1F << 20)
#define DIV_DDR_EN              (0x01 << 19)
#define DIV_DDR                 (0x1F << 14)

#define DIV_DC_SHIFT            (26)
#define DIV_CPU_SHIFT           (20)
#define DIV_DDR_SHIFT           (14)

static  INT32   _G_iPLLFreq;
static  INT32   _G_iCPUFreq;
static  INT32   _G_iDDRFreq;
static  INT32   _G_iDCFreq;
static  INT32   _G_iAPBFreq;

/*********************************************************************************************************
** 函数名称: ls1xClockInit
** 功能描述: probe requencies
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT ls1xClockInit()
{
    UINT32  iPll;
    UINT32  iCtrl;

    iPll     = readl(LS1X_CLK_PLL_FREQ);
    iCtrl    = readl(LS1X_CLK_PLL_DIV);

    /*
     * (12+PLL_FREQ[5:0]+ (PLL_FREQ[17:8]/1024))*33/2Mhz
     * Device Clock(SPI, I2C, PWM, CAN, WDT, UART) = DDR_CLK/2
     */
    _G_iPLLFreq = (((12 + (iPll & 0x3F) + (((iPll >> 8) & 0x3FF) >> 10)) * APB_CLK) >> 1);
    _G_iCPUFreq = _G_iPLLFreq / ((iCtrl & DIV_CPU) >> DIV_CPU_SHIFT);
    _G_iDDRFreq = _G_iPLLFreq / ((iCtrl & DIV_DDR) >> DIV_DDR_SHIFT);
    _G_iDCFreq  = _G_iPLLFreq / ((iCtrl & DIV_DC)  >> DIV_DC_SHIFT);
    _G_iAPBFreq = (_G_iDDRFreq >> 1);

    _PrintFormat("ls1xClockInit(): _G_iPLLFreq = %d, _G_iCPUFreq = %d, _G_iDDRFreq = %d, _G_iAPBFreq = %d\r\n",
            _G_iPLLFreq, _G_iCPUFreq, _G_iDDRFreq, _G_iAPBFreq);

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: ls1xPLLClock
** 功能描述: PLL Clock
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 ls1xPLLClock()
{
    return (_G_iPLLFreq);
}

/*********************************************************************************************************
** 函数名称: ls1xCPUClock
** 功能描述: CPU Clock
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 ls1xCPUClock()
{
    return (_G_iCPUFreq);
}

/*********************************************************************************************************
** 函数名称: ls1xDDRClock
** 功能描述: DDR Clock
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 ls1xDDRClock()
{
    return (_G_iDDRFreq);
}

/*********************************************************************************************************
** 函数名称: ls1xDCClock
** 功能描述: DC Clock
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 ls1xDCClock()
{
    return (_G_iDCFreq);
}

/*********************************************************************************************************
** 函数名称: ls1xAPBClock
** 功能描述: APB Clock
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 ls1xAPBClock()
{
    return (_G_iAPBFreq);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
