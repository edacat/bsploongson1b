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
** 文   件   名: ls1x_touch.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2016 年 01 月 07 日
**
** 描        述: Loongson1B 处理器 LED 驱动头文件部分
*********************************************************************************************************/


#ifndef LS1X_LED_H_
#define LS1X_LED_H_

/*********************************************************************************************************
** 函数名称: ls1xLedDrv
** 功能描述: ls1x LED 驱动初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ls1xLedDrv (VOID);

/*********************************************************************************************************
** 函数名称: ls1xLedDevCreate
** 功能描述: 创建 LED 设备
** 输　入  : NONE
**           pcName   设备名
**           uiGpio   GPIO Num
**           bIsOutPutHigh 初始电平
** 输　出  : 错误号
*********************************************************************************************************/
INT  ls1xLedDevCreate (CPCHAR  cpcName, UINT  uiGpio, BOOL  bIsOutPutHigh);

#endif                                                                  /*  LS1X_LED_H_                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
