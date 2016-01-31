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
** ��   ��   ��: ls1x_rtc.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 12 �� 09 ��
**
** ��        ��: loongson1B RTC ����Դ�ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "time.h"
#include <linux/compat.h>
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define LS1X_RTC_BASE       (0xBFE64000)
#define SYS_TOYTRIM         (LS1X_RTC_BASE + 0x20)
#define SYS_TOYWRITE0       (LS1X_RTC_BASE + 0x24)
#define SYS_TOYWRITE1       (LS1X_RTC_BASE + 0x28)
#define SYS_TOYREAD0        (LS1X_RTC_BASE + 0x2C)
#define SYS_TOYREAD1        (LS1X_RTC_BASE + 0x30)
#define SYS_TOYMATCH0       (LS1X_RTC_BASE + 0x34)
#define SYS_TOYMATCH1       (LS1X_RTC_BASE + 0x38)
#define SYS_TOYMATCH2       (LS1X_RTC_BASE + 0x3C)
#define SYS_COUNTER_CNTRL   (LS1X_RTC_BASE + 0x40)
#define SYS_RTCTRIM         (LS1X_RTC_BASE + 0x60)
#define SYS_RTCWRITE0       (LS1X_RTC_BASE + 0x64)
#define SYS_RTCREAD0        (LS1X_RTC_BASE + 0x68)
#define SYS_RTCMATCH0       (LS1X_RTC_BASE + 0x6C)
#define SYS_RTCMATCH1       (LS1X_RTC_BASE + 0x70)
#define SYS_RTCMATCH2       (LS1X_RTC_BASE + 0x74)

#define SYS_CNTRL_ERS       (1 << 23)
#define SYS_CNTRL_RTS       (1 << 20)
#define SYS_CNTRL_RM2       (1 << 19)
#define SYS_CNTRL_RM1       (1 << 18)
#define SYS_CNTRL_RM0       (1 << 17)
#define SYS_CNTRL_RS        (1 << 16)
#define SYS_CNTRL_BP        (1 << 14)
#define SYS_CNTRL_REN       (1 << 13)
#define SYS_CNTRL_BRT       (1 << 12)
#define SYS_CNTRL_TEN       (1 << 11)
#define SYS_CNTRL_BTT       (1 << 10)
#define SYS_CNTRL_E0        (1 << 8)
#define SYS_CNTRL_ETS       (1 << 7)
#define SYS_CNTRL_32S       (1 << 5)
#define SYS_CNTRL_TTS       (1 << 4)
#define SYS_CNTRL_TM2       (1 << 3)
#define SYS_CNTRL_TM1       (1 << 2)
#define SYS_CNTRL_TM0       (1 << 1)
#define SYS_CNTRL_TS        (1 << 0)
#define LS1X_RTC_CNTRL_INIT_VAL (SYS_CNTRL_BP | SYS_CNTRL_BRT | SYS_CNTRL_TEN | (1 << 9))
#define LS1X_RTC_CNTR_OK    (SYS_CNTRL_E0 | SYS_CNTRL_32S)

#define LS1X_SEC_OFFSET     (4)
#define LS1X_MIN_OFFSET     (10)
#define LS1X_HOUR_OFFSET    (16)
#define LS1X_DAY_OFFSET     (21)
#define LS1X_MONTH_OFFSET   (26)

#define LS1X_SEC_MASK       (0x3F)
#define LS1X_MIN_MASK       (0x3F)
#define LS1X_HOUR_MASK      (0x1F)
#define LS1X_DAY_MASK       (0x1F)
#define LS1X_MONTH_MASK     (0x3F)
#define LS1X_YEAR_MASK      (0xFFFFFFFF)

#define ls1x_get_sec(t)     (((t) >> LS1X_SEC_OFFSET) & LS1X_SEC_MASK)
#define ls1x_get_min(t)     (((t) >> LS1X_MIN_OFFSET) & LS1X_MIN_MASK)
#define ls1x_get_hour(t)    (((t) >> LS1X_HOUR_OFFSET) & LS1X_HOUR_MASK)
#define ls1x_get_day(t)     (((t) >> LS1X_DAY_OFFSET) & LS1X_DAY_MASK)
#define ls1x_get_month(t)   (((t) >> LS1X_MONTH_OFFSET) & LS1X_MONTH_MASK)

/*********************************************************************************************************
** ��������: ls1xRtcGet
** ��������: ��ȡ RTC ʱ��
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xRtcGet (PLW_RTC_FUNCS  pRtcFuncs, time_t  *pTimeNow)
{
    UINT32      uiVal = 0;
    struct tm   tmNow;

    uiVal = readl(SYS_TOYREAD0);

    tmNow.tm_sec  = ls1x_get_sec(uiVal);
    tmNow.tm_min  = ls1x_get_min(uiVal);
    tmNow.tm_hour = ls1x_get_hour(uiVal);
    tmNow.tm_mday = ls1x_get_day(uiVal);
    tmNow.tm_mon  = ls1x_get_month(uiVal) - 1;
    tmNow.tm_wday = (tmNow.tm_mday + 4 ) & 0x07;

    uiVal = readl(SYS_TOYREAD1);
    tmNow.tm_year = uiVal - 1900;

    if (tmNow.tm_year < 50) {
        tmNow.tm_year += 100;
    }

    if (pTimeNow) {
        *pTimeNow = timegm(&tmNow);
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xRtcSet
** ��������: ���� RTC ʱ��ֵ
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xRtcSet (PLW_RTC_FUNCS  pRtcFuncs, time_t  *pTimeNow)
{
    INTREG          iregInterLevel;
    struct tm       tmNow;
    UINT32          uiVal = 0;

    gmtime_r(pTimeNow, &tmNow);                                         /*  ת���� tm ʱ���ʽ          */

    API_InterLock(&iregInterLevel);

    uiVal  = (tmNow.tm_sec  << LS1X_SEC_OFFSET)
            |(tmNow.tm_min  << LS1X_MIN_OFFSET)
            |(tmNow.tm_hour << LS1X_HOUR_OFFSET)
            |(tmNow.tm_mday << LS1X_DAY_OFFSET)
            |((tmNow.tm_mon + 1)  << LS1X_MONTH_OFFSET);

    writel(uiVal, SYS_TOYWRITE0);

    uiVal = tmNow.tm_year + 1900;
    writel(uiVal, SYS_TOYWRITE1);

    API_InterUnlock(iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xRtcInit
** ��������: ETC HW Init
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ls1xRtcInit (VOID)
{
    UINT32  uiVal = 0;

    writel(LS1X_RTC_CNTRL_INIT_VAL, SYS_COUNTER_CNTRL);                 /*  ��ʼ��RTC���ƼĴ���ʹ��TOY  */

    uiVal = readl(SYS_COUNTER_CNTRL);
    if (!(uiVal & LS1X_RTC_CNTR_OK)) {
        return;
    }

    /*
     * Check �����Ƿ�Ϊ32.768K, ��ʱ����Ҫ��1Hz
     */
    if (readl(SYS_TOYTRIM) != 32767) {
        uiVal = 0x100000;
        while ((readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_TTS) && --uiVal) {
            usleep(100);
        }
        writel(32767, SYS_TOYTRIM);
    }

    while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_TTS);                   /*  �ٴ�ȷ��CLK����OK           */
}

static LW_RTC_FUNCS     _G_ls1xRtcFuncs = {
        ls1xRtcInit,
        ls1xRtcSet,
        ls1xRtcGet,
        LW_NULL
};

/*********************************************************************************************************
** ��������: ls1xRtcGetFuncs
** ��������: ��ȡ RTC ��������
** �䡡��  : NONE
** �䡡��  : RTC ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_RTC_FUNCS  ls1xRtcGetFuncs (VOID)
{
    return (&_G_ls1xRtcFuncs);
}

/*********************************************************************************************************
  END
*********************************************************************************************************/

