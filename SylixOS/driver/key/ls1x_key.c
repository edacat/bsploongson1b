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
** 文   件   名: ls1x_key.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2016 年 1 月 24 日
**
** 描        述: loongson1B Key(DM74LS165N) 驱动文件.
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <linux/compat.h>
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define HIGH                    (1)
#define LOW                     (0)
#define KEYNUM                  (16)
#define TIMEOUT                 (50)
/*********************************************************************************************************
 KEY 任务默认优先级
*********************************************************************************************************/
#define KEY_THREAD_PRIO         LW_PRIO_HIGH
/*********************************************************************************************************
  ls1x KEY 设备结构体定义
*********************************************************************************************************/
typedef struct key_dev {
    LW_DEV_HDR                  KEY_devhdr;                             /*  设备头                      */
    LW_LIST_LINE_HEADER         KEY_fdNodeHeader;
    LW_OBJECT_HANDLE            KEY_hThread;                            /*  Get Key Data 线程           */
    LW_OBJECT_HANDLE            KEY_hSemaphoreB;                        /*  信号量                      */
    UINT                        KEY_uiKeyEN;                            /*  KEY Enable GPIO NO.         */
    UINT                        KEY_uiKeySCL;                           /*  KEY Clock  GPIO NO.         */
    UINT                        KEY_uiKeyData;                          /*  KEY Data   GPIO NO.         */
    BOOL                        KEY_bQuit;                              /*  内线线程退出通知标志        */
    time_t                      KEY_time;                               /*  设备创建时间                */
} LS1X_KEY_DEV, *PLS1XKEY_DEV;
/*********************************************************************************************************
全局变量
*********************************************************************************************************/
static  INT                     _G_ils1xKeyDrvNum = 0;                  /*  KEY 主设备号                */
INT                             _G_iLastStateVal  = 0;

/*********************************************************************************************************
** 函数名称: ls1xKeyRead
** 功能描述: 读 ADC 设备
** 输　入  : pFdEntry              文件结构
**           pcBuffer              缓冲区
**           stMaxBytes            缓冲区长度
** 输　出  : 成功读取的字节数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  ls1xKeyRead (PLW_FD_ENTRY  pFdEntry, PCHAR pcBuffer, size_t  stMaxBytes)
{
    PLS1XKEY_DEV    pkey        = (PLS1XKEY_DEV)pFdEntry->FDENTRY_pdevhdrHdr;
    INT            *piBuffer;

    if (!pFdEntry || !pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }

    stMaxBytes = stMaxBytes / sizeof(INT);

    piBuffer   = (INT *)pcBuffer;

    API_SemaphoreBPost(pkey->KEY_hSemaphoreB);

   *piBuffer++ = _G_iLastStateVal;

   return  (sizeof(INT));
}
/*********************************************************************************************************
** 函数名称: ls1xKeyThread
** 功能描述: Key thread
** 输　入  : pkey     Key 设备地址
** 输　出  : 错误号
*********************************************************************************************************/
static VOID  ls1xKeyThread (PLS1XKEY_DEV  pkey)
{
    INT     iIndex;
    INT     iKeyVal;
    INT     iStateVal;
    for (;;) {
        if (API_SemaphoreBPend(pkey->KEY_hSemaphoreB, LW_OPTION_WAIT_INFINITE) == ERROR_NONE) {
            if (pkey->KEY_bQuit) {
                break;
            }
            gpioSetValue(pkey->KEY_uiKeySCL, LOW);
            gpioSetValue(pkey->KEY_uiKeyEN,  LOW);
            gpioSetValue(pkey->KEY_uiKeyEN,  HIGH);

            for (iIndex = 0, iStateVal = 0; iIndex < KEYNUM; iIndex++) {
                iKeyVal = gpioGetValue(pkey->KEY_uiKeyData);
                iStateVal |= (iKeyVal << iIndex);
                gpioSetValue(pkey->KEY_uiKeySCL, LOW);
                gpioSetValue(pkey->KEY_uiKeySCL, HIGH);
            }

            _G_iLastStateVal = iStateVal;

            API_TimeMSleep(TIMEOUT);
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "ls1xKeyThread(): led semaphore error.\r\n");
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: ls1xKeyOpen
** 功能描述: Key 打开
** 输　入  : pkey         Key 设备地址
**           pcName       设备名
**           iFlags       标志
**           iMode        模式
** 输　出  : 错误号
*********************************************************************************************************/
static LONG  ls1xKeyOpen (PLS1XKEY_DEV  pkey, CHAR  *pcName, INT iFlags, INT iMode)
{
    LW_CLASS_THREADATTR     threadattr;
    PLW_FD_NODE             pfdnode;
    BOOL                    bIsNew;
    INT                     iError;
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
        pfdnode = API_IosFdNodeAdd(&pkey->KEY_fdNodeHeader, (dev_t)pkey, 0,
                                          iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pfdnode == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "ls1xKeyOpen(): failed to add fd node!\r\n");
            return  (PX_ERROR);
        }
        if (LW_DEV_INC_USE_COUNT(&pkey->KEY_devhdr) == 1) {

            iError = gpioRequest(pkey->KEY_uiKeyEN, "KEYEN");
            if(iError != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "KEY Enable GPIO Request Failed.\r\n");
                goto ERR_FREEKEYEN;
            }
            iError = gpioDirectionOutput(pkey->KEY_uiKeyEN, HIGH);
            if(iError != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "KEY Enable GPIO Set  Direction Failed.\r\n");
                goto ERR_FREEKEYEN;
            }

            iError = gpioRequest(pkey->KEY_uiKeySCL, "KEYSCL");
            if(iError != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "KEY Clock GPIO Request Failed.\r\n");
                goto ERR_FREEKEYSCL;
            }
            iError = gpioDirectionOutput(pkey->KEY_uiKeySCL, HIGH);
            if(iError != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "KEY Clock GPIO Set  Direction Failed.\r\n");
                goto ERR_FREEKEYSCL;
            }

            iError = gpioRequest(pkey->KEY_uiKeyData, "KEYDATA");
            if(iError != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "KEY Data GPIO Request Failed.\r\n");
                goto ERR_FREEKEYDATA;
            }
            iError = gpioDirectionInput(pkey->KEY_uiKeyData);
            if(iError != ERROR_NONE) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "KEY Data GPIO Set  Direction Failed.\r\n");
                goto ERR_FREEKEYDATA;
            }

            pkey->KEY_bQuit = LW_FALSE;
            threadattr = API_ThreadAttrGetDefault();
            threadattr.THREADATTR_pvArg = (VOID *)pkey;
            threadattr.THREADATTR_ucPriority = KEY_THREAD_PRIO;
            threadattr.THREADATTR_ulOption  |= LW_OPTION_OBJECT_GLOBAL;
            pkey->KEY_hSemaphoreB = API_SemaphoreBCreate("sem_led", LW_FALSE,
                                                         LW_OPTION_DEFAULT,
                                                         LW_NULL);
            pkey->KEY_hThread     = API_ThreadCreate("t_led",
                                                     (PTHREAD_START_ROUTINE)ls1xKeyThread,
                                                     &threadattr, LW_NULL);
        }
        return  ((LONG)pfdnode);
    }
ERR_FREEKEYDATA:
        gpioFree(pkey->KEY_uiKeyData);
ERR_FREEKEYSCL:
        gpioFree(pkey->KEY_uiKeySCL);
ERR_FREEKEYEN:
        gpioFree(pkey->KEY_uiKeyEN);
        LW_DEV_DEC_USE_COUNT(&pkey->KEY_devhdr);
        API_IosFdNodeDec(&pkey->KEY_fdNodeHeader, pfdnode);
        return  (PX_ERROR);
}
/******************************************************************************
** 函数名称: ls1xKeyClose
** 功能描述: Key 关闭
** 输　入  : pfdentry 设备表结构地址
** 输　出  : 错误号
******************************************************************************/
static INT  ls1xKeyClose (PLW_FD_ENTRY   pfdentry)
{
    PLS1XKEY_DEV    pkey        = (PLS1XKEY_DEV)pfdentry->FDENTRY_pdevhdrHdr;
    PLW_FD_NODE     pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;

    if (pfdentry && pfdnode) {
        API_IosFdNodeDec(&pkey->KEY_fdNodeHeader, pfdnode);
        if (LW_DEV_DEC_USE_COUNT(&pkey->KEY_devhdr) == 0) {
            pkey->KEY_bQuit = LW_TRUE;
            API_SemaphoreBPostEx(pkey->KEY_hSemaphoreB, (VOID *)0);
            API_ThreadJoin(pkey->KEY_hThread, LW_NULL);
            pkey->KEY_hThread = 0;

            API_SemaphoreBDelete(&pkey->KEY_hSemaphoreB);
            pkey->KEY_hSemaphoreB = 0;

            gpioFree(pkey->KEY_uiKeyEN);
            gpioFree(pkey->KEY_uiKeySCL);
            gpioFree(pkey->KEY_uiKeyData);
        }
    }
    return  (ERROR_NONE);
}
/******************************************************************************
** 函数名称: ls1xKeyIoctl
** 功能描述: Key 控制
** 输　入  : pfdentry 设备表结构地址
**           iCmd     命令
**           lArg     参数
** 输　出  : 错误号
******************************************************************************/
static INT  ls1xKeyIoctl (PLW_FD_ENTRY   pfdentry, INT  iCmd, LONG  lArg)
{
    PLS1XKEY_DEV    pkey = (PLS1XKEY_DEV)pfdentry->FDENTRY_pdevhdrHdr;
    struct stat    *pstat;

    switch (iCmd) {
    case FIOFSTATGET:
        pstat = (struct stat *)lArg;
        if (pstat) {
            pstat->st_dev       = (dev_t)pkey;
            pstat->st_ino       = (ino_t)0;
            pstat->st_mode      = (S_IRUSR | S_IRGRP | S_IROTH);
            pstat->st_nlink     = 1;
            pstat->st_uid       = 0;
            pstat->st_gid       = 0;
            pstat->st_rdev      = 0;
            pstat->st_size      = 0;
            pstat->st_blksize   = 0;
            pstat->st_blocks    = 0;
            pstat->st_atime     = pkey->KEY_time;
            pstat->st_mtime     = pkey->KEY_time;
            pstat->st_ctime     = pkey->KEY_time;
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
        return  (ERROR_NONE);
    case 1:
        API_ThreadSetPriority(pkey->KEY_hThread, (UINT8)lArg);
        return  (ERROR_NONE);
    }
    return  (PX_ERROR);
}
/******************************************************************************
** 函数名称: ls1xKeyLstat
** 功能描述: 获得 Key 设备状态
** 输　入  : pDev     设备地址
**           pcName   设备名
**           pstat    参数
** 输　出  : 错误号
******************************************************************************/
static INT  ls1xKeyLstat (PLW_DEV_HDR  pDev, PCHAR  pcName, struct stat  *pstat)
{
    PLS1XKEY_DEV      pkey = (PLS1XKEY_DEV)pDev;

    if (pstat) {
        pstat->st_dev       = (dev_t)pkey;
        pstat->st_ino       = (ino_t)0;
        pstat->st_mode      = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pstat->st_nlink     = 1;
        pstat->st_uid       = 0;
        pstat->st_gid       = 0;
        pstat->st_rdev      = 0;
        pstat->st_size      = 0;
        pstat->st_blksize   = 0;
        pstat->st_blocks    = 0;
        pstat->st_atime     = pkey->KEY_time;
        pstat->st_mtime     = pkey->KEY_time;
        pstat->st_ctime     = pkey->KEY_time;
        return  (ERROR_NONE);
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ls1xKeyDrv
** 功能描述: ls1x Key 驱动初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ls1xKeyDrv (VOID)
{
    struct file_operations      fileop;

    if (_G_ils1xKeyDrvNum) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileop, 0, sizeof(struct file_operations));

    fileop.owner        = THIS_MODULE;
    fileop.fo_create    = ls1xKeyOpen;
    fileop.fo_open      = ls1xKeyOpen;
    fileop.fo_close     = ls1xKeyClose;
    fileop.fo_lstat     = ls1xKeyLstat;
    fileop.fo_ioctl     = ls1xKeyIoctl;
    fileop.fo_read      = ls1xKeyRead;

    _G_ils1xKeyDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_ils1xKeyDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_ils1xKeyDrvNum,      "Ryan.xin");
    DRIVER_DESCRIPTION(_G_ils1xKeyDrvNum, "ls1x key driver.");

    return  (_G_ils1xKeyDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}

/*********************************************************************************************************
** 函数名称: ls1xKeyDevCreate
** 功能描述: 创建 Key 设备
** 输　入  : NONE
**           pcName    设备名
**           uiKeyEN   key Enable GPIO Number
**           uiKeyCLK  Key Clock  GPIO Number
**           uiKeyData Key Data   GPIO NUmber
** 输　出  : 错误号
*********************************************************************************************************/
INT  ls1xKeyDevCreate (CPCHAR  cpcName, UINT  uiKeyEN, UINT  uiKeyCLK, UINT  uiKeyData)
{
    PLS1XKEY_DEV  pkey;

    pkey = (PLS1XKEY_DEV)__SHEAP_ALLOC(sizeof(LS1X_KEY_DEV));
    if (!pkey) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);
    }

    lib_memset(pkey, 0, sizeof(LS1X_KEY_DEV));

    pkey->KEY_uiKeyEN   = uiKeyEN;
    pkey->KEY_uiKeySCL  = uiKeyCLK;
    pkey->KEY_uiKeyData = uiKeyData;
    pkey->KEY_time      = time(LW_NULL);

    if (API_IosDevAddEx(&pkey->KEY_devhdr, cpcName, _G_ils1xKeyDrvNum, DT_CHR) != ERROR_NONE) {
        __SHEAP_FREE(pkey);
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
