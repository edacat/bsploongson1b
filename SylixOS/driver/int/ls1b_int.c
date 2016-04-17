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
** ��   ��   ��: ls1b_int.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 11 �� 30 ��
**
** ��        ��: Loongson1b �������ж�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
/*********************************************************************************************************
  �жϿ���������ַ
*********************************************************************************************************/
static addr_t   _G_aulIntCtrlBase[4] = {
        BSP_CFG_INT0_BASE,
        BSP_CFG_INT1_BASE,
        BSP_CFG_INT2_BASE,
        BSP_CFG_INT3_BASE,
};
/*********************************************************************************************************
  �Ĵ���ƫ��
*********************************************************************************************************/
#define INTISR                  (0x00)                                  /*  �жϿ���״̬�Ĵ���          */
#define INTIEN                  (0x04)                                  /*  �жϿ���ʹ�ܼĴ���          */
#define INTSET                  (0x08)                                  /*  �ж���λ�Ĵ���              */
#define INTCLR                  (0x0C)                                  /*  �ж���ռĴ���              */
#define INTPOL                  (0x10)                                  /*  �ߵ�ƽ�����ж�ʹ�ܼĴ���    */
#define INTEDGE                 (0x14)                                  /*  ���ش����ж�ʹ�ܼĴ���      */
/*********************************************************************************************************
** ��������: ls1bIntInit
** ��������: �ж�ϵͳ��ʼ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1bIntHandle
** ��������: �ж����
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1bIntVectorEnable
** ��������: ʹ��ָ�����ж�����
** ��  ��  : ulVector     �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1bIntVectorDisable
** ��������: ����ָ�����ж�����
** ��  ��  : ulVector     �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1bIntVectorIsEnable
** ��������: ���ָ�����ж������Ƿ�ʹ��
** ��  ��  : ulVector     �ж�����
** ��  ��  : LW_FALSE �� LW_TRUE
** ȫ�ֱ���:
** ����ģ��:
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
