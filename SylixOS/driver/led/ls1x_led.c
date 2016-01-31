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
** 描        述: Loongson1B 处理器 LED 驱动文件部分
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <linux/compat.h>
/*********************************************************************************************************
  LED 简易操作宏
*********************************************************************************************************/
#define LED_ON(pled)            gpioSetValue(pled->LED_uiGpio, pled->LED_bIsOutPutHigh ? 1 : 0)
#define LED_OFF(pled)           gpioSetValue(pled->LED_uiGpio, pled->LED_bIsOutPutHigh ? 0 : 1)
/*********************************************************************************************************
 LED 任务默认优先级
*********************************************************************************************************/
#define LED_THREAD_PRIO         LW_PRIO_HIGH
/*********************************************************************************************************
全局变量
*********************************************************************************************************/
static  INT                     _G_ils1xLedDrvNum = 0;                  /*  LED 主设备号                */
/*********************************************************************************************************
  ls1x LED 设备结构体定义
*********************************************************************************************************/
typedef struct led_dev {
    LW_DEV_HDR                  LED_devhdr;                             /*  设备头                      */
    LW_LIST_LINE_HEADER         LED_fdNodeHeader;
    LW_OBJECT_HANDLE            LED_hThread;                            /*  闪烁线程                    */
    LW_OBJECT_HANDLE            LED_hSemaphoreB;                        /*  信号量                      */
    UINT                        LED_uiGpio;                             /*  GPIO                        */
    BOOL                        LED_bIsOutPutHigh;                      /*  是否输出高电平为点亮        */
    BOOL                        LED_bQuit;                              /*  内线线程退出通知标志        */
    time_t                      LED_time;                               /*  设备创建时间                */
} LS1X_LED_DEV, *PLS1XLED_DEV;

/*********************************************************************************************************
** 函数名称: ls1xLedThread
** 功能描述: LED thread
** 输　入  : pled     LED 设备地址
** 输　出  : 错误号
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
** 函数名称: ls1xLedOpen
** 功能描述: LED 打开
** 输　入  : pled         LED 设备地址
**           pcName       设备名
**           iFlags       标志
**           iMode        模式
** 输　出  : 错误号
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
** 函数名称: ls1xLedClose
** 功能描述: LED 关闭
** 输　入  : pfdentry 设备表结构地址
** 输　出  : 错误号
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
** 函数名称: ls1xLedIoctl
** 功能描述: LED 控制
** 输　入  : pfdentry 设备表结构地址
**           iCmd     命令
**           lArg     参数
** 输　出  : 错误号
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
** 函数名称: ls1xLedLstat
** 功能描述: 获得 LED 设备状态
** 输　入  : pDev     设备地址
**           pcName   设备名
**           pstat    参数
** 输　出  : 错误号
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
** 函数名称: ls1xLedDrv
** 功能描述: 创建 LED 驱动程序
** 输　入  : NONE
** 输　出  : 错误号
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
** 函数名称: ls1xLedDevCreate
** 功能描述: 创建 LED 设备
** 输　入  : NONE
**           pcName   设备名
**           uiGpio   GPIO Num
**           bIsOutPutHigh 初始电平
** 输　出  : 错误号
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
