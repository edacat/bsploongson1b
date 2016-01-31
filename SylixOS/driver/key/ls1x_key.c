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
** ��   ��   ��: ls1x_key.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2016 �� 1 �� 24 ��
**
** ��        ��: loongson1B Key(DM74LS165N) �����ļ�.
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <linux/compat.h>
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define HIGH                    (1)
#define LOW                     (0)
#define KEYNUM                  (16)
#define TIMEOUT                 (50)
/*********************************************************************************************************
 KEY ����Ĭ�����ȼ�
*********************************************************************************************************/
#define KEY_THREAD_PRIO         LW_PRIO_HIGH
/*********************************************************************************************************
  ls1x KEY �豸�ṹ�嶨��
*********************************************************************************************************/
typedef struct key_dev {
    LW_DEV_HDR                  KEY_devhdr;                             /*  �豸ͷ                      */
    LW_LIST_LINE_HEADER         KEY_fdNodeHeader;
    LW_OBJECT_HANDLE            KEY_hThread;                            /*  Get Key Data �߳�           */
    LW_OBJECT_HANDLE            KEY_hSemaphoreB;                        /*  �ź���                      */
    UINT                        KEY_uiKeyEN;                            /*  KEY Enable GPIO NO.         */
    UINT                        KEY_uiKeySCL;                           /*  KEY Clock  GPIO NO.         */
    UINT                        KEY_uiKeyData;                          /*  KEY Data   GPIO NO.         */
    BOOL                        KEY_bQuit;                              /*  �����߳��˳�֪ͨ��־        */
    time_t                      KEY_time;                               /*  �豸����ʱ��                */
} LS1X_KEY_DEV, *PLS1XKEY_DEV;
/*********************************************************************************************************
ȫ�ֱ���
*********************************************************************************************************/
static  INT                     _G_ils1xKeyDrvNum = 0;                  /*  KEY ���豸��                */
INT                             _G_iLastStateVal  = 0;

/*********************************************************************************************************
** ��������: ls1xKeyRead
** ��������: �� ADC �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           pcBuffer              ������
**           stMaxBytes            ����������
** �䡡��  : �ɹ���ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1xKeyThread
** ��������: Key thread
** �䡡��  : pkey     Key �豸��ַ
** �䡡��  : �����
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
** ��������: ls1xKeyOpen
** ��������: Key ��
** �䡡��  : pkey         Key �豸��ַ
**           pcName       �豸��
**           iFlags       ��־
**           iMode        ģʽ
** �䡡��  : �����
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
** ��������: ls1xKeyClose
** ��������: Key �ر�
** �䡡��  : pfdentry �豸��ṹ��ַ
** �䡡��  : �����
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
** ��������: ls1xKeyIoctl
** ��������: Key ����
** �䡡��  : pfdentry �豸��ṹ��ַ
**           iCmd     ����
**           lArg     ����
** �䡡��  : �����
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
** ��������: ls1xKeyLstat
** ��������: ��� Key �豸״̬
** �䡡��  : pDev     �豸��ַ
**           pcName   �豸��
**           pstat    ����
** �䡡��  : �����
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
** ��������: ls1xKeyDrv
** ��������: ls1x Key ������ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: ls1xKeyDevCreate
** ��������: ���� Key �豸
** �䡡��  : NONE
**           pcName    �豸��
**           uiKeyEN   key Enable GPIO Number
**           uiKeyCLK  Key Clock  GPIO Number
**           uiKeyData Key Data   GPIO NUmber
** �䡡��  : �����
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
