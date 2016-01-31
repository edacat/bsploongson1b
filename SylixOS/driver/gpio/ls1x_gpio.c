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
** ��   ��   ��: ls1x_gpio.h
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 12 �� 10 ��
**
** ��        ��: LS1X GPIO ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"                                                    /*  ����ϵͳ                    */
#include <linux/compat.h>

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define LS1X_GPIO0_BASE_ADDR    (0xBFD010C0)
#define LS1X_GPIO1_BASE_ADDR    (0xBFD010C4)
#define LS1X_GPIO2_BASE_ADDR    (0xBFD010C8)
#define LS1X_GPIO3_BASE_ADDR    (0xBFD010CC)

#define LS1X_GPIO_CFG_OF        (0)                                     /*  ���üĴ���                  */
#define LS1X_GPIO_OE_OF         (0x10)                                  /*  ����ʹ�ܼĴ���              */
#define LS1X_GPIO_IN_OF         (0x20)                                  /*  ����Ĵ���                  */
#define LS1X_GPIO_OUT_OF        (0x30)                                  /*  ����Ĵ���                  */

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
  GPIO����������ַ
*********************************************************************************************************/
static addr_t   _G_GPIOBase[] = {
        LS1X_GPIO0_BASE_ADDR,
        LS1X_GPIO1_BASE_ADDR,
        LS1X_GPIO2_BASE_ADDR,
        LS1X_GPIO3_BASE_ADDR,
};
/*********************************************************************************************************
** ��������: ls1xGpioGetDirection
** ��������: ���ָ�� GPIO ����
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ���� 1:���
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1xGpioGet
** ��������: ���ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: �͵�ƽ 1:�ߵ�ƽ
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1xGpioSet
** ��������: ����ָ�� GPIO ��ƽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1xGpioDirectionInput
** ��������: ����ָ�� GPIO Ϊ����ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1xGpioDirectionOutput
** ��������: ����ָ�� GPIO Ϊ���ģʽ
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           iValue      �����ƽ
** ��  ��  : 0: ��ȷ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1xGpioSetupIrq
** ��������: ����ָ�� GPIO Ϊ�ⲿ�ж�����ܽ�
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
**           bIsLevel    �Ƿ�Ϊ��ƽ����
**           uiType      ���Ϊ��ƽ����, 1 ��ʾ�ߵ�ƽ����, 0 ��ʾ�͵�ƽ����
**                       ���Ϊ���ش���, 1 ��ʾ�����ش���, 0 ��ʾ�½��ش���, 2 ˫���ش���
** ��  ��  : IRQ ������ -1:����
** ȫ�ֱ���:
** ����ģ��:
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
     * �����Ӧ��������
     */
    ulVector = BSP_CFG_INT_SUB_VECTOR((uiBank + 2), uiPinID);

    if (bIsLevel) {
        /*
         *  ��ƽ����
         */
        writel(0xFFFFFFFF, uiBank == 0 ? BSP_CFG_INT2_BASE + 0x10: BSP_CFG_INT3_BASE + 0x10);

    } else {
        /*
         *  ���ش���
         */
        writel(0x0000E000, uiBank == 0 ? BSP_CFG_INT2_BASE + 0x1C: BSP_CFG_INT3_BASE + 0x1C);
    }

    return  (ulVector);
}
/*********************************************************************************************************
** ��������: ls1xGpioClearIrq
** ��������: ���ָ�� GPIO �жϱ�־
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls1xGpioClearIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT    uiBank;
    UINT    uiPinID;
    ULONG   ulVector;

    uiBank  = PINID_TO_BANK(uiOffset);
    uiPinID = PINID_TO_PIN(uiOffset);

    /*
     * �����Ӧ��������
     */
    ulVector = BSP_CFG_INT_SUB_VECTOR((uiBank + 2), uiPinID);

    API_InterVectorDisable(ulVector);
}
/*********************************************************************************************************
** ��������: ls1xGpioSvrIrq
** ��������: �ж� GPIO �жϱ�־
** ��  ��  : pGpioChip   GPIO оƬ
**           uiOffset    GPIO ��� BASE ��ƫ����
** ��  ��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  ls1xGpioSvrIrq (PLW_GPIO_CHIP  pGpioChip, UINT  uiOffset)
{
    UINT    uiBank;
    UINT    uiPinID;
    ULONG   ulVector;

    uiBank  = PINID_TO_BANK(uiOffset);
    uiPinID = PINID_TO_PIN(uiOffset);

    /*
     * �����Ӧ��������
     */
    ulVector = BSP_CFG_INT_SUB_VECTOR((uiBank + 2), uiPinID);

    if (bspIntVectorIsEnable(ulVector)) {
        return  (LW_IRQ_HANDLED);
    } else {
        return (LW_IRQ_NONE);
    }
}
/*********************************************************************************************************
  GPIO ��������
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
** ��������: ls1xGpioDrv
** ��������: ���� ls1x GPIO ����
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ls1xGpioDrv (VOID)
{
    gpioChipAdd(&_G_ls1xGpioChip);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

