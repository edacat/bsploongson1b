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
** ��   ��   ��: ls1x_touch.h
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2016 �� 01 �� 07 ��
**
** ��        ��: Loongson1B ������ LED �����ļ�����
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <linux/compat.h>
/*********************************************************************************************************
  LED ���ײ�����
*********************************************************************************************************/
#define LED_ON(pled)            gpioSetValue(pled->LED_uiGpio, pled->LED_bIsOutPutHigh ? 1 : 0)
#define LED_OFF(pled)           gpioSetValue(pled->LED_uiGpio, pled->LED_bIsOutPutHigh ? 0 : 1)
/*********************************************************************************************************
 LED ����Ĭ�����ȼ�
*********************************************************************************************************/
#define LED_THREAD_PRIO         LW_PRIO_HIGH
/*********************************************************************************************************
ȫ�ֱ���
*********************************************************************************************************/
static  INT                     _G_ils1xLedDrvNum = 0;                  /*  LED ���豸��                */
/*********************************************************************************************************
  ls1x LED �豸�ṹ�嶨��
*********************************************************************************************************/
typedef struct led_dev {
    LW_DEV_HDR                  LED_devhdr;                             /*  �豸ͷ                      */
    LW_LIST_LINE_HEADER         LED_fdNodeHeader;
    LW_OBJECT_HANDLE            LED_hThread;                            /*  ��˸�߳�                    */
    LW_OBJECT_HANDLE            LED_hSemaphoreB;                        /*  �ź���                      */
    UINT                        LED_uiGpio;                             /*  GPIO                        */
    BOOL                        LED_bIsOutPutHigh;                      /*  �Ƿ�����ߵ�ƽΪ����        */
    BOOL                        LED_bQuit;                              /*  �����߳��˳�֪ͨ��־        */
    time_t                      LED_time;                               /*  �豸����ʱ��                */
} LS1X_LED_DEV, *PLS1XLED_DEV;

/*********************************************************************************************************
** ��������: ls1xLedThread
** ��������: LED thread
** �䡡��  : pled     LED �豸��ַ
** �䡡��  : �����
*********************************************************************************************************/
static VOID  ls1xLedThread (PLS1XLED_DEV  pled)
{
    ULONG   ulMsTime;
    for (;;) {
        if (API_SemaphoreBPendEx(pled->LED_hSemaphoreB, LW_OPTION_WAIT_INFINITE,
                (PVOID *)&ulMsTime) == ERROR_NONE) {
            if (pled->LED_bQuit) {
                break;
            }
            LED_ON(pled);
            API_TimeMSleep(ulMsTime);
            LED_OFF(pled);
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "ls1xLedThread(): led semaphore error!\r\n");
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: ls1xLedOpen
** ��������: LED ��
** �䡡��  : pled         LED �豸��ַ
**           pcName       �豸��
**           iFlags       ��־
**           iMode        ģʽ
** �䡡��  : �����
*********************************************************************************************************/
static LONG  ls1xLedOpen (PLS1XLED_DEV  pled, CHAR  *pcName, INT iFlags, INT iMode)
{
    LW_CLASS_THREADATTR     threadattr;
    PLW_FD_NODE             pfdnode;
    BOOL                    bIsNew;
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
        pfdnode = API_IosFdNodeAdd(&pled->LED_fdNodeHeader, (dev_t)pled, 0,
                                          iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pfdnode == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "ls1xLedOpen(): failed to add fd node!\r\n");
            return  (PX_ERROR);
        }
        if (LW_DEV_INC_USE_COUNT(&pled->LED_devhdr) == 1) {
            if (gpioRequestOne(pled->LED_uiGpio,
                    (pled->LED_bIsOutPutHigh ? LW_GPIOF_OUT_INIT_LOW :LW_GPIOF_OUT_INIT_HIGH), "led")) {
                LW_DEV_DEC_USE_COUNT(&pled->LED_devhdr);
                API_IosFdNodeDec(&pled->LED_fdNodeHeader, pfdnode);
                return  (PX_ERROR);
            }

            pled->LED_bQuit = LW_FALSE;
            threadattr = API_ThreadAttrGetDefault();
            threadattr.THREADATTR_pvArg = (VOID *)pled;
            threadattr.THREADATTR_ucPriority = LED_THREAD_PRIO;
            threadattr.THREADATTR_ulOption  |= LW_OPTION_OBJECT_GLOBAL;
            pled->LED_hSemaphoreB = API_SemaphoreBCreate("sem_led", LW_FALSE,
                                                         LW_OPTION_DEFAULT,
                                                         LW_NULL);
            pled->LED_hThread     = API_ThreadCreate("t_led",
                                                     (PTHREAD_START_ROUTINE)ls1xLedThread,
                                                     &threadattr, LW_NULL);
        }
        return  ((LONG)pfdnode);
    }
}
/*********************************************************************************************************
** ��������: ls1xLedClose
** ��������: LED �ر�
** �䡡��  : pfdentry �豸��ṹ��ַ
** �䡡��  : �����
*********************************************************************************************************/
static INT  ls1xLedClose (PLW_FD_ENTRY   pfdentry)
{
    PLS1XLED_DEV    pled        = (PLS1XLED_DEV)pfdentry->FDENTRY_pdevhdrHdr;
    PLW_FD_NODE     pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;

    if (pfdentry && pfdnode) {
        API_IosFdNodeDec(&pled->LED_fdNodeHeader, pfdnode);
        if (LW_DEV_DEC_USE_COUNT(&pled->LED_devhdr) == 0) {
            pled->LED_bQuit = LW_TRUE;
            API_SemaphoreBPostEx(pled->LED_hSemaphoreB, (VOID *)0);
            API_ThreadJoin(pled->LED_hThread, LW_NULL);
            pled->LED_hThread = 0;

            API_SemaphoreBDelete(&pled->LED_hSemaphoreB);
            pled->LED_hSemaphoreB = 0;

            gpioFree(pled->LED_uiGpio);
        }
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xLedIoctl
** ��������: LED ����
** �䡡��  : pfdentry �豸��ṹ��ַ
**           iCmd     ����
**           lArg     ����
** �䡡��  : �����
*********************************************************************************************************/
static INT  ls1xLedIoctl (PLW_FD_ENTRY   pfdentry, INT  iCmd, LONG  lArg)
{
    PLS1XLED_DEV    pled = (PLS1XLED_DEV)pfdentry->FDENTRY_pdevhdrHdr;
    struct stat    *pstat;

    switch (iCmd) {
    case FIOFSTATGET:
        pstat = (struct stat *)lArg;
        if (pstat) {
            pstat->st_dev       = (dev_t)pled;
            pstat->st_ino       = (ino_t)0;
            pstat->st_mode      = (S_IRUSR | S_IRGRP | S_IROTH);
            pstat->st_nlink     = 1;
            pstat->st_uid       = 0;
            pstat->st_gid       = 0;
            pstat->st_rdev      = 0;
            pstat->st_size      = 0;
            pstat->st_blksize   = 0;
            pstat->st_blocks    = 0;
            pstat->st_atime     = pled->LED_time;
            pstat->st_mtime     = pled->LED_time;
            pstat->st_ctime     = pled->LED_time;
            return  (ERROR_NONE);
        }
        return  (PX_ERROR);
    case FIOSETFL:
        if ((int)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    case 0:
        API_SemaphoreBPostEx(pled->LED_hSemaphoreB, (VOID *)lArg);
        return  (ERROR_NONE);
    case 1:
        API_ThreadSetPriority(pled->LED_hThread, (UINT8)lArg);
        return  (ERROR_NONE);
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls1xLedLstat
** ��������: ��� LED �豸״̬
** �䡡��  : pDev     �豸��ַ
**           pcName   �豸��
**           pstat    ����
** �䡡��  : �����
*********************************************************************************************************/
static INT  ls1xLedLstat (PLW_DEV_HDR  pDev, PCHAR  pcName, struct stat  *pstat)
{
    PLS1XLED_DEV      pled = (PLS1XLED_DEV)pDev;

    if (pstat) {
        pstat->st_dev       = (dev_t)pled;
        pstat->st_ino       = (ino_t)0;
        pstat->st_mode      = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pstat->st_nlink     = 1;
        pstat->st_uid       = 0;
        pstat->st_gid       = 0;
        pstat->st_rdev      = 0;
        pstat->st_size      = 0;
        pstat->st_blksize   = 0;
        pstat->st_blocks    = 0;
        pstat->st_atime     = pled->LED_time;
        pstat->st_mtime     = pled->LED_time;
        pstat->st_ctime     = pled->LED_time;
        return  (ERROR_NONE);
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls1xLedDrv
** ��������: ���� LED ��������
** �䡡��  : NONE
** �䡡��  : �����
*********************************************************************************************************/
INT  ls1xLedDrv (VOID)
{
    struct file_operations      fileop;

    if (_G_ils1xLedDrvNum) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileop, 0, sizeof(struct file_operations));

    fileop.owner        = THIS_MODULE;
    fileop.fo_create    = ls1xLedOpen;
    fileop.fo_open      = ls1xLedOpen;
    fileop.fo_close     = ls1xLedClose;
    fileop.fo_lstat     = ls1xLedLstat;
    fileop.fo_ioctl     = ls1xLedIoctl;

    _G_ils1xLedDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_ils1xLedDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_ils1xLedDrvNum,      "Ryan.xin");
    DRIVER_DESCRIPTION(_G_ils1xLedDrvNum, "ls1x led driver.");

    return  (_G_ils1xLedDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** ��������: ls1xLedDevCreate
** ��������: ���� LED �豸
** �䡡��  : NONE
**           pcName   �豸��
**           uiGpio   GPIO Num
**           bIsOutPutHigh ��ʼ��ƽ
** �䡡��  : �����
*********************************************************************************************************/
INT  ls1xLedDevCreate (CPCHAR  cpcName, UINT  uiGpio, BOOL  bIsOutPutHigh)
{
    PLS1XLED_DEV  pled;

    pled = (PLS1XLED_DEV)__SHEAP_ALLOC(sizeof(LS1X_LED_DEV));
    if (!pled) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);
    }

    lib_memset(pled, 0, sizeof(LS1X_LED_DEV));

    pled->LED_uiGpio        = uiGpio;
    pled->LED_bIsOutPutHigh = bIsOutPutHigh;
    pled->LED_time          = time(LW_NULL);

    if (API_IosDevAddEx(&pled->LED_devhdr, cpcName, _G_ils1xLedDrvNum, DT_CHR) != ERROR_NONE) {
        __SHEAP_FREE(pled);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);
    }

    _ErrorHandle(ERROR_NONE);
    return    (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
