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
** ��   ��   ��: ls1x_touch.c
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2016 �� 01 �� 05 ��
**
** ��        ��: Loongson1B ������ Touch �����ļ�����(ads7846)
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
#include "SylixOS.h"
#include "ls1x_touch.h"
#include "mouse.h"
#include "../spi/ls1x_spi.h"
/*********************************************************************************************************
  �ܽ���غ궨��
*********************************************************************************************************/
#define ADS7846A_PENIRQ             (60)

#define MARTERID                    (0)                                 /*  SPI Bus Number              */
#define DEVICEID                    (1)                                 /*  Device chip select ID       */
#define DEVICEENABLE                (1)                                 /*  Enable Device               */
#define DEVICEDISABLE               (0)                                 /*  Disbale Device              */
#define DEVICESPEEDHZ               (2500000)

#define ADS7846AIN_Y                (0x91)                              /*  ADS7843�����ֲɼ�X��ѹ      */
#define ADS7846AIN_X                (0xD1)                              /*  ADS7843�����ֲɼ�Y��ѹ      */
#define ADS7846AIN_Z1               (0xB1)                              /*  ADS7843�����ֲɼ�Z1��ѹ     */
#define ADS7846AIN_Z2               (0xC1)                              /*  ADS7843�����ֲɼ�Z2��ѹ     */
#define ADS7846AIN_PWRDOWN          (0x90)
/*********************************************************************************************************
  ���������Ժ궨��
*********************************************************************************************************/
#define RxTOUCH_PLATE               (595)                               /*  ������X+��X-֮��ĵ���      */
#define RyTOUCH_PLATE               (358)                               /*  ������Y+��Y-֮��ĵ���      */
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define ADC_SAMPLE_NUM              (5)                                 /*  ��������                    */
/*********************************************************************************************************
  ����ֵ����
*********************************************************************************************************/
#define ADC_X_MAX_DIFF              (60)                                /*  X������ֵ                 */
#define ADC_Y_MAX_DIFF              (90)                                /*  Y������ֵ                 */
/*********************************************************************************************************
  ѹ������ֵ����
*********************************************************************************************************/
#define PRESS_Z_MAX                 (1500)                              /*  �������ѹ������ֵ          */
#define PRESS_Z1_MIN                (65)                                /*  ������Z1��Сֵ              */
/*********************************************************************************************************
  ɨ������Ĭ�����ȼ�
*********************************************************************************************************/
#define ads7846_THREAD_PRIO         LW_PRIO_LOW
/*********************************************************************************************************
  ads7846 �豸�ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                      ADS7846_devhdr;                     /*  �豸ͷ                      */
    touchscreen_event_notify        ADS7846_tData;                      /*  �ɼ���������                */
    BOOL                            ADS7846_bIsReadRel;                 /*  �Ƿ��ȡ�� release ����     */
    LW_HANDLE                       ADS7846_hThread;                    /*  ɨ���߳�                    */
    LW_SEL_WAKEUPLIST               ADS7846_selwulList;                 /*  select() �ȴ���             */
    LW_SPINLOCK_DEFINE              (ADS7846_slLock);                   /*  ������                      */
} ADS7846_DEV;
/*********************************************************************************************************
  ����ȫ�ֱ���
*********************************************************************************************************/
static  INT         _G_iADS7846DevNum = PX_ERROR;
PLW_SPI_DEVICE      _G_pSpiDevice;

/*********************************************************************************************************
** Function name:           ads7846Init
** Descriptions:            ��ʼ�� ads7846
** input parameters:        NONE
**
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
*********************************************************************************************************/
static VOID  ads7846Init (VOID)
{
    API_GpioRequest(ADS7846A_PENIRQ, "ads7846penirq");
    API_GpioDirectionInput(ADS7846A_PENIRQ);

    _G_pSpiDevice = API_SpiDeviceCreate("/bus/spi/0", "/dev/ads7846");
    if (_G_pSpiDevice == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "failed to create spi device!\r\n");
        API_SpiDeviceDelete(_G_pSpiDevice);
    }
}
/*********************************************************************************************************
** Function name:           measurementGet
** Descriptions:            ������������ϵĵ�ѹ
** input parameters:        NONE
** output parameters:       pX      X �᷽���ѹ
**                          pY      Y �᷽���ѹ
**
** Returned value:          ��ȡ���ĵ�ѹֵ
*********************************************************************************************************/
static VOID measurementGet (INT  *pX, INT  *pY)
{
    UCHAR           pucTXBuffer[3][1];
    UCHAR           pucRXBuffer[3][2];
    LW_SPI_MESSAGE  spiMsg[10];
    UINT            uiIndex;

    lib_bzero(&spiMsg, sizeof(spiMsg));

    pucTXBuffer[0][0] = ADS7846AIN_Y;
    /*
     * �ӵ���һ�Σ��ڶ��ζ�ȡΪ׼
     */
    for(uiIndex = 0; uiIndex < 4; uiIndex+= 2) {
        spiMsg[uiIndex].SPIMSG_pucWrBuffer = pucTXBuffer[0];
        spiMsg[uiIndex].SPIMSG_pucRdBuffer = LW_NULL;
        spiMsg[uiIndex].SPIMSG_uiLen       = 1;

        spiMsg[uiIndex + 1].SPIMSG_pucWrBuffer = LW_NULL;
        spiMsg[uiIndex + 1].SPIMSG_pucRdBuffer = pucRXBuffer[0];
        spiMsg[uiIndex + 1].SPIMSG_uiLen       = 2;
    }

    pucTXBuffer[1][0] = ADS7846AIN_X;
    /*
     * �ӵ���һ�Σ��ڶ��ζ�ȡΪ׼
     */
    for(uiIndex = 4; uiIndex < 8; uiIndex+= 2) {
        spiMsg[uiIndex].SPIMSG_pucWrBuffer = pucTXBuffer[1];
        spiMsg[uiIndex].SPIMSG_pucRdBuffer = LW_NULL;
        spiMsg[uiIndex].SPIMSG_uiLen       = 1;

        spiMsg[uiIndex + 1].SPIMSG_pucWrBuffer = LW_NULL;
        spiMsg[uiIndex + 1].SPIMSG_pucRdBuffer = pucRXBuffer[1];
        spiMsg[uiIndex + 1].SPIMSG_uiLen       = 2;
    }

    pucTXBuffer[2][0] = ADS7846AIN_PWRDOWN;
    spiMsg[8].SPIMSG_pucWrBuffer = pucTXBuffer[2];
    spiMsg[8].SPIMSG_pucRdBuffer = LW_NULL;
    spiMsg[8].SPIMSG_uiLen       = 1;

    spiMsg[9].SPIMSG_pucWrBuffer = LW_NULL;
    spiMsg[9].SPIMSG_pucRdBuffer = pucRXBuffer[2];
    spiMsg[9].SPIMSG_uiLen       = 2;

    if (API_SpiDeviceBusRequest(_G_pSpiDevice) != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "failed to bus request!\r\n");
        API_SpiDeviceDelete(_G_pSpiDevice);
    }

    ls1xSpipfuncChipSelect(MARTERID, DEVICEID, DEVICEENABLE);

    API_SpiDeviceCtl(_G_pSpiDevice, LW_SPI_CTL_BAUDRATE, DEVICESPEEDHZ);

    if (API_SpiDeviceTransfer(_G_pSpiDevice, spiMsg, 10) != 10) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "failed to spi transfer!\r\n");
        API_SpiDeviceDelete(_G_pSpiDevice);
    }

    ls1xSpipfuncChipSelect(MARTERID, DEVICEID, DEVICEDISABLE);

    if (API_SpiDeviceBusRelease(_G_pSpiDevice) != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "failed to bus release!\r\n");
        API_SpiDeviceDelete(_G_pSpiDevice);
    }

    *pY = (((pucRXBuffer[0][0] << 4) | (pucRXBuffer[0][1] >> 4)) >> 3);
    *pX = (((pucRXBuffer[1][0] << 4) | (pucRXBuffer[1][1] >> 4)) >> 3);
}
/*********************************************************************************************************
** Function name:           sortPoint
** Descriptions:            �����������С��������
** input parameters:        piBuf   ������
**                          iNum    ��Ҫ���������
**
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
static  VOID    sortPoint (INT  *piBuf, INT  iNum)
{
    INT     i, j;
    INT     iTmp;

    for (i = 1; i < iNum; i++) {
        for (j = 0; j < iNum - i; j++) {                                /*  ����                        */

            if (piBuf[j] > piBuf[j + 1]) {                              /*  ����                        */
                iTmp         = piBuf[j];
                piBuf[j]     = piBuf[j + 1];
                piBuf[j + 1] = iTmp;
            }
        }
    }
}
/*********************************************************************************************************
** Function name:           touchGetXY
** Descriptions:            ��ô����������ѹ X Y
** input parameters:        NONE
** output parameters:       pX      X �᷽���ѹ
**                          pY      Y �᷽���ѹ
** Returned value:          1: ��ʾ����Ч   0: ��ʾ����Ч
** Descriptions             ����ads7846������� X �� Y ��Ķ�β�������������ֵ��Ϊ����ѹ����Χ�ڵ�ֵ
*********************************************************************************************************/
static INT    touchGetXY (INT  *pX, INT  *pY)
{
    INT             i;

    INT             iXTmp[ADC_SAMPLE_NUM];                              /*  ��ʱ����                  */
    INT             iYTmp[ADC_SAMPLE_NUM];

    INT             iXDiff, iYDiff;                                     /*  X Y ���������ֵ          */

    if (!pX || !pY) {                                                   /*  ָ�����                    */
        return  (0);
    }

    *pX = 0;                                                            /*  ��ʼ��Ϊ��Ч��              */
    *pY = 0;

    if (API_GpioGetValue(ADS7846A_PENIRQ)) {
        return  (0);
    }

    for (i = 0; i < ADC_SAMPLE_NUM; i++) {                              /*  ��������                    */
        measurementGet(&iYTmp[i], &iXTmp[i]);
    }

    sortPoint(iXTmp, ADC_SAMPLE_NUM);                                   /*  ������                      */
    sortPoint(iYTmp, ADC_SAMPLE_NUM);

    iXDiff = iXTmp[ADC_SAMPLE_NUM - 2] - iXTmp[1];
    iYDiff = iYTmp[ADC_SAMPLE_NUM - 2] - iYTmp[1];                      /*  ��������ֵ                */
                                                                        /*  �����㷨ȥ�����ֵ����Сֵ  */

    if (iXDiff > ADC_X_MAX_DIFF) {                                      /*  X ����Ч����                */
        return  (0);
    }

    if (iYDiff > ADC_Y_MAX_DIFF) {                                      /*  Y ����Ч����                */
        return  (0);
    }

    *pX = iXTmp[ADC_SAMPLE_NUM >> 1];
    *pY = iYTmp[ADC_SAMPLE_NUM >> 1];                                   /*  ����ƽ��ֵ                  */

    return  (1);
}
/*********************************************************************************************************
** Function name:           ads7846Thread
** Descriptions:            ads7846     ɨ���߳�
** input parameters:        ADS7846_DEV      ���ƽṹ
**
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
VOID  ads7846Thread (ADS7846_DEV  *pads7846Dev)
{
    INT             ix, iy;
    INTREG          iregInterLevel;

    while (1) {
        if (touchGetXY(&ix, &iy)) {
            /*
             *  ��ǰ�е������.
             */
#undef  __TOUCH_DEBUG
#ifdef  __TOUCH_DEBUG
#define COM2    0xBFE48000
            {
                CHAR    cBuf[2][20];
                sprintf(cBuf[0], "Y: %X    ", iy);
                sprintf(cBuf[1], "X: %X\r\n", ix);
                uart8250PutStr(COM2, cBuf[0]);
                uart8250PutStr(COM2, cBuf[1]);
            }
#endif
            LW_SPIN_LOCK_QUICK(&pads7846Dev->ADS7846_slLock, &iregInterLevel);
            pads7846Dev->ADS7846_tData.kstat |= MOUSE_LEFT;
            pads7846Dev->ADS7846_tData.xmovement = ix;
            pads7846Dev->ADS7846_tData.ymovement = iy;
            LW_SPIN_UNLOCK_QUICK(&pads7846Dev->ADS7846_slLock, iregInterLevel);
            SEL_WAKE_UP_ALL(&pads7846Dev->ADS7846_selwulList, SELREAD); /*  �ͷ����еȴ������߳�        */
        } else {
            /*
             *  ��ǰû�е������.
             */
            if (pads7846Dev->ADS7846_tData.kstat & MOUSE_LEFT) {
                LW_SPIN_LOCK_QUICK(&pads7846Dev->ADS7846_slLock, &iregInterLevel);
                pads7846Dev->ADS7846_tData.kstat &= (~MOUSE_LEFT);
                LW_SPIN_UNLOCK_QUICK(&pads7846Dev->ADS7846_slLock, iregInterLevel);
            }

            if (pads7846Dev->ADS7846_bIsReadRel == LW_FALSE) {          /*  û�ж�ȡ���ͷŲ���          */
            SEL_WAKE_UP_ALL(&pads7846Dev->ADS7846_selwulList, SELREAD); /*  �ͷ����еȴ������߳�        */
            }
        }

        API_TimeMSleep(30);
    }
}
/*********************************************************************************************************
** Function name:           ads7846Ioctl
** Descriptions:            CAN �豸����
** input parameters:        pads7846Dev      ads7846 �豸
**                          iCmd             ��������
**                          lArg             ����
**
** output parameters:       NONE
** Returned value:          ERROR_NONE or PX_ERROR
*********************************************************************************************************/
static INT ads7846Ioctl (ADS7846_DEV  *pads7846Dev, INT  iCmd, LONG  lArg)
{
    PLW_SEL_WAKEUPNODE   pselwunNode;
    INT                  iError = ERROR_NONE;

    switch (iCmd) {

    case ADS7846_SET_PRIO:
         API_ThreadSetPriority(pads7846Dev->ADS7846_hThread, (UINT8)lArg);
         break;

    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&pads7846Dev->ADS7846_selwulList, pselwunNode);
        break;

    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&pads7846Dev->ADS7846_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
        break;

    default:
        iError = PX_ERROR;
        errno  = ENOSYS;
        break;
    }

     return  (iError);
}
/*********************************************************************************************************
** Function name:           ads7846Open
** Descriptions:            ads7846 �豸��
** input parameters:        pads7846Dev          ads7846 �豸
**                          pcName               �豸����
**                          iFlags               ���豸ʱʹ�õı�־
**                          iMode                �򿪵ķ�ʽ������
** output parameters:       NONE
** Returned value:          ERROR CODE
*********************************************************************************************************/
static LONG  ads7846Open (ADS7846_DEV  *pads7846Dev,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode)
{
    LW_CLASS_THREADATTR     threadattr;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return    (PX_ERROR);
    }

    if (LW_DEV_INC_USE_COUNT(&pads7846Dev->ADS7846_devhdr) == 1) {

        ads7846Init();

        pads7846Dev->ADS7846_tData.ctype   = MOUSE_CTYPE_ABS;
        pads7846Dev->ADS7846_tData.kstat   = 0;
        pads7846Dev->ADS7846_tData.xanalog = 0;
        pads7846Dev->ADS7846_tData.yanalog = 0;

        pads7846Dev->ADS7846_bIsReadRel  = LW_TRUE;

        threadattr = API_ThreadAttrGetDefault();
        threadattr.THREADATTR_pvArg      = (void *)pads7846Dev;
        threadattr.THREADATTR_ucPriority = ads7846_THREAD_PRIO;
        threadattr.THREADATTR_ulOption  |= LW_OPTION_OBJECT_GLOBAL;

        pads7846Dev->ADS7846_hThread     = API_ThreadCreate("ads7846_ts",
                                                           (PTHREAD_START_ROUTINE)ads7846Thread,
                                                            &threadattr, LW_NULL);
        return  ((LONG)pads7846Dev);
    }

    _ErrorHandle(ERROR_IO_FILE_EXIST);
    return    (PX_ERROR);
}
/*********************************************************************************************************
** Function name:           ads7846Close
** Descriptions:            ads7846 �豸�ر�
** input parameters:        pads7846Dev         ads7846 �豸
**
** output parameters:       NONE
** Returned value:          ERROR CODE
*********************************************************************************************************/
static INT ads7846Close (ADS7846_DEV  *pads7846Dev)
{
    if (LW_DEV_GET_USE_COUNT(&pads7846Dev->ADS7846_devhdr)) {
        if (!LW_DEV_DEC_USE_COUNT(&pads7846Dev->ADS7846_devhdr)) {

            SEL_WAKE_UP_ALL(&pads7846Dev->ADS7846_selwulList,
                            SELEXCEPT);                                 /*  �����쳣�ȴ�                */
            SEL_WAKE_UP_ALL(&pads7846Dev->ADS7846_selwulList,
                            SELWRITE);                                  /*  �����쳣�ȴ�                */
            SEL_WAKE_UP_ALL(&pads7846Dev->ADS7846_selwulList,
                            SELREAD);                                   /*  �����쳣�ȴ�                */
            API_ThreadDelete(&pads7846Dev->ADS7846_hThread, LW_NULL);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** Function name:           ads7846Read
** Descriptions:            �� ads7846 �豸
** input parameters:        pads7846Dev      ads7846 �豸
**                          pTOUCHData       ������ָ��
**                          stNbyte          ��������С�ֽ���
**
** output parameters:       NONE
** Returned value:          ERROR CODE
*********************************************************************************************************/
static ssize_t ads7846Read (ADS7846_DEV               *pads7846Dev,
                              touchscreen_event_notify    *pTOUCHData,
                              size_t                       stNbyte)
{
    INTREG               iregInterLevel;

    if (stNbyte == 0) {
        return  (0);
    }

    LW_SPIN_LOCK_QUICK(&pads7846Dev->ADS7846_slLock, &iregInterLevel);

    pTOUCHData->ctype   = pads7846Dev->ADS7846_tData.ctype;
    pTOUCHData->kstat   = pads7846Dev->ADS7846_tData.kstat;
    pTOUCHData->xanalog = pads7846Dev->ADS7846_tData.xanalog;
    pTOUCHData->yanalog = pads7846Dev->ADS7846_tData.yanalog;

    if (pads7846Dev->ADS7846_tData.kstat & MOUSE_LEFT) {                /*  ��ȡ������¼�              */
        pads7846Dev->ADS7846_bIsReadRel = LW_FALSE;                     /*  ��Ҫȷ��Ӧ�ö����ͷŲ���    */
    } else {
        pads7846Dev->ADS7846_bIsReadRel = LW_TRUE;                      /*  �Ѿ���ȡ���ͷŲ���          */
    }

    LW_SPIN_UNLOCK_QUICK(&pads7846Dev->ADS7846_slLock, iregInterLevel);

    return  (sizeof(touchscreen_event_notify));
}
/*********************************************************************************************************
** Function name:           ads7846Drv
** Descriptions:            ��װ ads7846 ��������
** input parameters:
**
** output parameters:       NONE
** Returned value:          ERROR CODE
*********************************************************************************************************/
INT  ads7846Drv (void)
{
    if (_G_iADS7846DevNum > 0) {
        return  (ERROR_NONE);
    }

    _G_iADS7846DevNum = iosDrvInstall(ads7846Open, LW_NULL, ads7846Open, ads7846Close,
                                      ads7846Read, LW_NULL, ads7846Ioctl);

    DRIVER_LICENSE(_G_iADS7846DevNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iADS7846DevNum,      "Ryan Xin");
    DRIVER_DESCRIPTION(_G_iADS7846DevNum, "ads7846 driver.");

    return  (_G_iADS7846DevNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** Function name:           ads7846Create
** Descriptions:            ���� ads7846 �豸
** input parameters:        pcName       �豸��
**
** output parameters:       NONE
** Returned value:          ERROR CODE
*********************************************************************************************************/
INT  ads7846DevCreate (PCHAR     pcName)
{
    ADS7846_DEV   *pads7846Dev;
    INT              iTemp;

    if (!pcName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return    (PX_ERROR);
    }

    if (_G_iADS7846DevNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ads7846 Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return    (PX_ERROR);
    }

    pads7846Dev = (ADS7846_DEV *)__SHEAP_ALLOC(sizeof(ADS7846_DEV));
    if (!pads7846Dev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);
    }

    lib_memset(pads7846Dev, 0, sizeof(ADS7846_DEV));

    SEL_WAKE_UP_LIST_INIT(&pads7846Dev->ADS7846_selwulList);            /*  ��ʼ�� select �ȴ���        */
    LW_SPIN_INIT(&pads7846Dev->ADS7846_slLock);                         /*  ��ʼ��������                */

    iTemp = (INT)iosDevAdd(&pads7846Dev->ADS7846_devhdr,
                           pcName,
                           _G_iADS7846DevNum);
    if (iTemp) {
        __SHEAP_FREE(pads7846Dev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "add device error.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);

    } else {
        _ErrorHandle(ERROR_NONE);
        return    (ERROR_NONE);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

