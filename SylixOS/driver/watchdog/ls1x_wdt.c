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
** 文   件   名: ls1x_wdt.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 12 月 13 日
**
** 描        述: loongson1B watchdog 驱动源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "string.h"
#include "config.h"
#include "../clock/ls1x_clock.h"
#include <linux/compat.h>
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define LS1X_CFG_WDT_BASE       (0xBFE5C060)
#define WDT_EN                  (0x00)
#define WDT_TIMER               (0x04)
#define WDT_SET                 (0x08)
#define TIMEOUT_MIN             (0)
#define TIMEOUT_MAX             (45)                                    /*  45 second                   */
#define TIMEOUT_DEFAULT         TIMEOUT_MAX
#define WDTDEBUG                (1)

/*********************************************************************************************************
全局变量
*********************************************************************************************************/
static  INT                     _G_ils1xWDTDrvNum = 0;                  /*  WDT 主设备号                */
/*********************************************************************************************************
  ls1x LED 设备结构体定义
*********************************************************************************************************/
typedef struct wdt_dev {
    LW_DEV_HDR                  WDT_devhdr;                             /*  设备头                      */
    LW_LIST_LINE_HEADER         WDT_fdNodeHeader;
    addr_t                      WDT_ulPhyAddrBase;                      /*  物理地址基地址              */
    time_t                      WDT_time;                               /*  设备创建时间                */
} LS1X_WDT_DEV, *PLS1X_WDT_DEV;

/*********************************************************************************************************
** 函数名称: ls1xWDTHwInit
** 功能描述: 初始化 WATCH_DOG 硬件设备
** 输　入  : pwdt    WATCH_DOG 设备
**           uiTimeout time out
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ls1xWDTHwInit (PLS1X_WDT_DEV pwdt, UINT uiTimeout)
{
    if (uiTimeout > TIMEOUT_MAX || uiTimeout < TIMEOUT_MIN) {
        uiTimeout = TIMEOUT_DEFAULT;
    }

    ULONG   uiWDTClock = ls1xAPBClock();

    writel(0x01, pwdt->WDT_ulPhyAddrBase + WDT_EN);
#if WDTDEBUG
    uiWDTClock = 1;
#else
#endif
    writel(uiTimeout * uiWDTClock, pwdt->WDT_ulPhyAddrBase + WDT_TIMER);
    writel(0x01, pwdt->WDT_ulPhyAddrBase + WDT_SET);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xWDTDeInit
** 功能描述: 关闭 WATCH_DOG 硬件设备
** 输　入  : pwdt    WATCH_DOG 设备
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ls1xWDTDeInit (PLS1X_WDT_DEV     pwdt)
{
#if WDTDEBUG
#else
    writel(0x00, pwdt->WDT_ulPhyAddrBase + WDT_EN);
#endif
}
/*********************************************************************************************************
** 函数名称: ls1xWDTOpen
** 功能描述: 打开 WATCH_DOG 设备
** 输　入  : pwdt         WATCH_DOG 设备地址
**           pcName       设备名
**           iFlags       标志
**           iMode        模式
** 输　出  : 错误号
*********************************************************************************************************/
static LONG  ls1xWDTOpen (PLS1X_WDT_DEV  pwdt, CHAR  *pcName, INT iFlags, INT iMode)
{
    PLW_FD_NODE             pfdnode;
    BOOL                    bIsNew;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
        pfdnode = API_IosFdNodeAdd(&pwdt->WDT_fdNodeHeader, (dev_t)pwdt, 0,
                                          iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pfdnode == LW_NULL) {
            printk(KERN_ERR "ls1xWDTOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }
        if (LW_DEV_INC_USE_COUNT(&pwdt->WDT_devhdr) == 1) {
            if (ls1xWDTHwInit(pwdt, TIMEOUT_DEFAULT) != ERROR_NONE) {
                LW_DEV_DEC_USE_COUNT(&pwdt->WDT_devhdr);
                API_IosFdNodeDec(&pwdt->WDT_fdNodeHeader, pfdnode);
                return  (PX_ERROR);
            }
        }
        return  ((LONG)pfdnode);
    }
}
/*********************************************************************************************************
** 函数名称: ls1xWDTClose
** 功能描述: 关闭 WATCH_DOG 设备
** 输　入  : pfdentry 设备表结构地址
** 输　出  : 错误号
*********************************************************************************************************/
static INT  ls1xWDTClose (PLW_FD_ENTRY   pfdentry)
{
    PLS1X_WDT_DEV   pwdt       = (PLS1X_WDT_DEV)pfdentry->FDENTRY_pdevhdrHdr;
    PLW_FD_NODE     pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;

    if (pfdentry && pfdnode) {
        API_IosFdNodeDec(&pwdt->WDT_fdNodeHeader, pfdnode);
        if (LW_DEV_DEC_USE_COUNT(&pwdt->WDT_devhdr) == 0) {
            ls1xWDTDeInit(pwdt);
        }
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ls1xWDTIoctl
** 功能描述: 控制 WATCH_DOG 设备
** 输　入  : pfdentry 设备表结构地址
**           iCmd     命令
**           lArg     参数
** 输　出  : 错误号
*********************************************************************************************************/
static INT  ls1xWDTIoctl (PLW_FD_ENTRY   pfdentry, INT  iCmd, LONG  lArg)
{
    PLS1X_WDT_DEV   pwdt = (PLS1X_WDT_DEV)pfdentry->FDENTRY_pdevhdrHdr;
    struct stat    *pstat;

    switch (iCmd) {
    case FIOFSTATGET:
        pstat = (struct stat *)lArg;
        if (pstat) {
            pstat->st_dev       = (dev_t)pwdt;
            pstat->st_ino       = (ino_t)0;
            pstat->st_mode      = (S_IRUSR | S_IRGRP | S_IROTH);
            pstat->st_nlink     = 1;
            pstat->st_uid       = 0;
            pstat->st_gid       = 0;
            pstat->st_rdev      = 0;
            pstat->st_size      = 0;
            pstat->st_blksize   = 0;
            pstat->st_blocks    = 0;
            pstat->st_atime     = pwdt->WDT_time;
            pstat->st_mtime     = pwdt->WDT_time;
            pstat->st_ctime     = pwdt->WDT_time;
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
        ls1xWDTDeInit(pwdt);
        return  (ERROR_NONE);
    case 1:
        ls1xWDTHwInit(pwdt, (UINT)lArg);
        return  (ERROR_NONE);
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ls1xWDTLstat
** 功能描述: 获得 WATCH_DOG 设备状态
** 输　入  : pDev     设备地址
**           pcName   设备名
**           pstat    参数
** 输　出  : 错误号
*********************************************************************************************************/
static INT  ls1xWDTLstat (PLW_DEV_HDR  pDev, PCHAR  pcName, struct stat  *pstat)
{
    PLS1X_WDT_DEV      pwdt = (PLS1X_WDT_DEV)pDev;

    if (pstat) {
        pstat->st_dev       = (dev_t)pwdt;
        pstat->st_ino       = (ino_t)0;
        pstat->st_mode      = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pstat->st_nlink     = 1;
        pstat->st_uid       = 0;
        pstat->st_gid       = 0;
        pstat->st_rdev      = 0;
        pstat->st_size      = 0;
        pstat->st_blksize   = 0;
        pstat->st_blocks    = 0;
        pstat->st_atime     = pwdt->WDT_time;
        pstat->st_mtime     = pwdt->WDT_time;
        pstat->st_ctime     = pwdt->WDT_time;
        return  (ERROR_NONE);
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ls1xWatchDogDrv
** 功能描述: 安装 WATCH_DOG 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ls1xWatchDogDrv (VOID)
{
    struct file_operations  fileop;

    if (_G_ils1xWDTDrvNum) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileop, 0, sizeof(struct file_operations));

    fileop.owner        = THIS_MODULE;
    fileop.fo_create    = ls1xWDTOpen;
    fileop.fo_open      = ls1xWDTOpen;
    fileop.fo_close     = ls1xWDTClose;
    fileop.fo_lstat     = ls1xWDTLstat;
    fileop.fo_ioctl     = ls1xWDTIoctl;

    _G_ils1xWDTDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_ils1xWDTDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_ils1xWDTDrvNum,      "Ryan.xin");
    DRIVER_DESCRIPTION(_G_ils1xWDTDrvNum, "ls1x watchdog driver.");

    return  (_G_ils1xWDTDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ls1xWatchDogDevAdd
** 功能描述: 创建 WATCH_DOG 设备
** 输　入  : pcName   设备名
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ls1xWatchDogDevAdd (CPCHAR  cpcName)
{
    PLS1X_WDT_DEV   pwdt;

    pwdt = (PLS1X_WDT_DEV)__SHEAP_ALLOC(sizeof(LS1X_WDT_DEV));
    if (!pwdt) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);
    }

    lib_memset(pwdt, 0, sizeof(LS1X_WDT_DEV));

    pwdt->WDT_ulPhyAddrBase = LS1X_CFG_WDT_BASE;
    pwdt->WDT_time          = time(LW_NULL);

    if (API_IosDevAddEx(&pwdt->WDT_devhdr, cpcName, _G_ils1xWDTDrvNum, DT_CHR) != ERROR_NONE) {
        __SHEAP_FREE(pwdt);
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


