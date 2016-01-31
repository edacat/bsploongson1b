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
** ��   ��   ��: bspInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 06 �� 25 ��
**
** ��        ��: BSP �û� C �������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
/*********************************************************************************************************
  ����ϵͳ���
*********************************************************************************************************/
#include "SylixOS.h"                                                    /*  ����ϵͳ                    */
#include "stdlib.h"                                                     /*  for system() function       */
#include "gdbserver.h"                                                  /*  GDB ������                  */
#include "sys/compiler.h"                                               /*  ���������                  */
#include "lwip/tcpip.h"                                                 /*  ����ϵͳ                    */
#include "netif/etharp.h"                                               /*  ��̫������ӿ�              */
#include "netif/aodvif.h"                                               /*  aodv ��������������ӿ�     */
/*********************************************************************************************************
  BSP �� ��������
*********************************************************************************************************/
#include "driver/16c550/16c550_sio.h"
#include "driver/mtd/k9f1g08.h"
#include "driver/designware_eth/designware_eth.h"
#include "driver/rtc/ls1x_rtc.h"
#include "driver/gpio/ls1x_gpio.h"
#include "driver/lcd/ls1x_lcd.h"
#include "driver/watchdog/ls1x_wdt.h"
#include "driver/i2c/ls1x_i2c.h"
#include "driver/spi/ls1x_spi.h"
#include "driver/clock/ls1x_clock.h"
#include "driver/touch/ls1x_touch.h"
#include "driver/led/ls1x_led.h"
#include "driver/sdi/spi_sdi.h"
#include "driver/key/ls1x_key.h"
#define  LS1XKEY
#undef   LS1XKEY
/*********************************************************************************************************
  ����ϵͳ���ű�
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0 && defined(__GNUC__)
#include "symbol.h"
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
                                                                        /*  defined(__GNUC__)           */
/*********************************************************************************************************
  �ڴ��ʼ��ӳ���
*********************************************************************************************************/
#define  __BSPINIT_MAIN_FILE
#include "bspMap.h"
/*********************************************************************************************************
  ���߳��������̶߳�ջ (t_boot ���Դ�һ��, startup.sh �п����кܶ����Ķ�ջ�Ĳ���)
*********************************************************************************************************/
#define  __LW_THREAD_BOOT_STK_SIZE      (16 * LW_CFG_KB_SIZE)
#define  __LW_THREAD_MAIN_STK_SIZE      (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  ���߳�����
*********************************************************************************************************/
VOID  t_main(VOID);
/*********************************************************************************************************
** ��������: halModeInit
** ��������: ��ʼ��Ŀ��ϵͳ���е�ģʽ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halModeInit (VOID)
{
}
/*********************************************************************************************************
** ��������: halTimeInit
** ��������: ��ʼ��Ŀ��ϵͳʱ��ϵͳ (ϵͳĬ��ʱ��Ϊ: ��8��)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static VOID  halTimeInit (VOID)
{
    PLW_RTC_FUNCS   prtcfuncs = ls1xRtcGetFuncs();

    rtcDrv();
    rtcDevCreate(prtcfuncs);                                            /*  ����Ӳ�� RTC �豸           */
    rtcToSys();                                                         /*  �� RTC ʱ��ͬ����ϵͳʱ��   */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** ��������: halIdleInit
** ��������: ��ʼ��Ŀ��ϵͳ����ʱ����ҵ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halIdleInit (VOID)
{

}
/*********************************************************************************************************
** ��������: halCacheInit
** ��������: Ŀ��ϵͳ CPU ���ٻ����ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

static VOID  halCacheInit (VOID)
{
    API_CacheLibInit(CACHE_COPYBACK, CACHE_COPYBACK, MIPS_MACHINE_LS1B);/*  ��ʼ�� CACHE ϵͳ           */
    API_CacheEnable(INSTRUCTION_CACHE);
    API_CacheEnable(DATA_CACHE);                                        /*  ʹ�� CACHE                  */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: halFpuInit
** ��������: Ŀ��ϵͳ FPU �������㵥Ԫ��ʼ��
** �䡡��: 	 NONE
** �䡡��:   NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

static VOID  halFpuInit (VOID)
{
    API_KernelFpuInit(MIPS_MACHINE_LS1B, MIPS_FPU_NONE);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: halPmInit
** ��������: ��ʼ��Ŀ��ϵͳ��Դ����ϵͳ
** �䡡��:   NONE
** �䡡��:   NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

static VOID  halPmInit (VOID)
{
    /*
     * TODO: ������Ĵ������, �ο���������:
     */
#if 0                                                                   /*  �ο����뿪ʼ                */
    PLW_PMA_FUNCS  pmafuncs = pmGetFuncs();

    pmAdapterCreate("inner_pm", 21, pmafuncs);
#endif                                                                  /*  �ο��������                */
}

#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
** ��������: halBusInit
** ��������: ��ʼ��Ŀ��ϵͳ����ϵͳ
** �䡡��: 	 NONE
** �䡡��:   NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halBusInit (VOID)
{
    PLW_SPI_FUNCS    pSpiFuncs;

    PLW_I2C_FUNCS    pI2cFuncs;

    pI2cFuncs  = i2cBusFuns(0);

    API_I2cLibInit();                                                   /*  ��ʼ�� i2c ��ϵͳ           */

    API_I2cAdapterCreate("/bus/i2c/0", pI2cFuncs, 10, 1);               /*  ���� i2c0 ����������        */

    pSpiFuncs = spiBusDrv(0);
    API_SpiLibInit();                                                   /*  ��ʼ�� spi �����           */
    if (pSpiFuncs) {
        API_SpiAdapterCreate("/bus/spi/0", pSpiFuncs);                  /*  ���� spi0 ����������        */
    }

}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halDrvInit
** ��������: ��ʼ��Ŀ��ϵͳ��̬��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDrvInit (VOID)
{
    /*
     *  standard device driver (rootfs and procfs need install first.)
     */
    rootFsDrv();                                                        /*  ROOT   device driver        */
    procFsDrv();                                                        /*  proc   device driver        */
    shmDrv();                                                           /*  shm    device driver        */
    randDrv();                                                          /*  random device driver        */
    ptyDrv();                                                           /*  pty    device driver        */
    ttyDrv();                                                           /*  tty    device driver        */
    memDrv();                                                           /*  mem    device driver        */
    pipeDrv();                                                          /*  pipe   device driver        */
    spipeDrv();                                                         /*  spipe  device driver        */
    fatFsDrv();                                                         /*  FAT FS device driver        */
    ramFsDrv();                                                         /*  RAM FS device driver        */
    romFsDrv();                                                         /*  ROM FS device driver        */
    nfsDrv();                                                           /*  nfs    device driver        */
    yaffsDrv();                                                         /*  yaffs  device driver        */
    canDrv();                                                           /*  CAN    device driver        */

    ls1xGpioDrv();

    ls1xWatchDogDrv();
    ls1xLedDrv();
    ads7846Drv();                                                       /*  ads7846 �������豸          */
    ls1xKeyDrv();

#if 0                                                                   /*  �ο����뿪ʼ                */
    INT              i;
    ULONG            ulMaxBytes;
    PLW_DMA_FUNCS    pdmafuncs;

    for (i = 0; i < 4; i++) {                                           /*  ��װ 4 ��ͨ�� DMA ͨ��      */
        pdmafuncs = dmaGetFuncs(LW_DMA_CHANNEL0 + i, &ulMaxBytes);
        dmaDrv((UINT)i, pdmafuncs, (size_t)ulMaxBytes);                 /*  ��װ DMA ����������         */
    }
#endif                                                                  /*  �ο��������                */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halDevInit
** ��������: ��ʼ��Ŀ��ϵͳ��̬�豸���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDevInit (VOID)
{
    /*
     *  �������ļ�ϵͳʱ, ���Զ����� dev, mnt, var Ŀ¼.
     */
    rootFsDevCreate();                                                  /*  �������ļ�ϵͳ              */
    procFsDevCreate();                                                  /*  ���� proc �ļ�ϵͳ          */
    shmDevCreate();                                                     /*  ���������ڴ��豸            */
    randDevCreate();                                                    /*  ����������ļ�              */

    SIO_CHAN    *psio0 = sioChan16C550Create(0);                        /*  �������� 0 ͨ��             */
    ttyDevCreate("/dev/ttyS0", psio0, 30, 50);                          /*  add    tty   device         */

    SIO_CHAN    *psio1 = sioChan16C550Create(1);                        /*  �������� 1 ͨ��             */
    ttyDevCreate("/dev/ttyS1", psio1, 30, 50);                          /*  add    tty   device         */

    SIO_CHAN    *psio2 = sioChan16C550Create(2);                        /*  �������� 2 ͨ��             */
    ttyDevCreate("/dev/ttyS2", psio2, 30, 50);                          /*  add    tty   device         */

    SIO_CHAN    *psio3 = sioChan16C550Create(3);                        /*  �������� 3 ͨ��             */
    ttyDevCreate("/dev/ttyS3", psio3, 30, 50);                          /*  add    tty   device         */

    SIO_CHAN    *psio4 = sioChan16C550Create(4);                        /*  �������� 4 ͨ��             */
    ttyDevCreate("/dev/ttyS4", psio4, 30, 50);                          /*  add    tty   device         */
#if 0
    SIO_CHAN    *psio5 = sioChan16C550Create(5);                        /*  �������� 5 ͨ��             */
    ttyDevCreate("/dev/ttyS5", psio5, 30, 50);                          /*  add    tty   device         */
#endif
    yaffsDevCreate("/yaffs2");                                          /*  create yaffs device(only fs)*/

    ls1xWatchDogDevAdd("/dev/wdt");

    ls1bFbDevCreate("/dev/fb0");                                        /*  create lcd device           */
    ads7846DevCreate("/dev/input/touch0");                              /*  ads7846 �������豸          */

#ifdef LS1XKEY
#define KEYENABLENUM    (38)
#define KEYECLOCKNUM    (39)
#define KEYDATANUM      (41)
    ls1xKeyDevCreate("/dev/key", KEYENABLENUM, KEYECLOCKNUM, KEYDATANUM);
#else
    ls1xLedDevCreate("/dev/led_green0", 38, LW_FALSE);
    ls1xLedDevCreate("/dev/led_green1", 39, LW_FALSE);
#endif
    ls1xLedDevCreate("/dev/led_buzzer", 40, LW_FALSE);

/*********************************************************************************************************
 * ����SD��ص���Ϣ
*********************************************************************************************************/
    static SPI_PLATFORM_OPS  _G_spiplatops;
#ifdef LS1XKEY
#define SDCDPIN SPI_SDI_PIN_NONE
#else
#define SDCDPIN (41)
#endif
#define SDWPPIN SPI_SDI_PIN_NONE
    _G_spiplatops.SPIPO_pfuncChipSelect = (PVOID)ls1xSpipfuncChipSelect;
    spisdiLibInit(&_G_spiplatops);
    spisdiDrvInstall(LS1XSPI0, SPI_CS2, SDCDPIN, SDWPPIN);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halLogInit
** ��������: ��ʼ��Ŀ��ϵͳ��־ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

static VOID  halLogInit (VOID)
{
    fd_set      fdLog;

    FD_ZERO(&fdLog);
    FD_SET(STD_OUT, &fdLog);
    API_LogFdSet(STD_OUT + 1, &fdLog);                                  /*  ��ʼ����־                  */
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
/*********************************************************************************************************
** ��������: halStdFileInit
** ��������: ��ʼ��Ŀ��ϵͳ��׼�ļ�ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halStdFileInit (VOID)
{
    int     iFd = open("/dev/ttyS2", O_RDWR, 0);

    if (iFd >= 0) {
        ioctl(iFd, FIOBAUDRATE,   SIO_BAUD_115200);
        ioctl(iFd, FIOSETOPTIONS, (OPT_TERMINAL & (~OPT_7_BIT)));       /*  system terminal 8 bit mode  */

        ioGlobalStdSet(STD_IN,  iFd);
        ioGlobalStdSet(STD_OUT, iFd);
        ioGlobalStdSet(STD_ERR, iFd);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halShellInit
** ��������: ��ʼ��Ŀ��ϵͳ shell ����, (getopt ʹ��ǰһ��Ҫ��ʼ�� shell ����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID  halShellInit (VOID)
{
    API_TShellInit();

    /*
     *  ��ʼ�� appl �м�� shell �ӿ�
     */
    zlibShellInit();
    viShellInit();

    /*
     *  ��ʼ�� GDB ������
     */
    gdbInit();
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: halVmmInit
** ��������: ��ʼ��Ŀ��ϵͳ�����ڴ�������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  halVmmInit (VOID)
{
    API_VmmLibInit(_G_zonedescGlobal, _G_globaldescMap, MIPS_MACHINE_LS1B);
    API_VmmMmuEnable();
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: halNetInit
** ��������: ���������ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

static VOID  halNetInit (VOID)
{
    API_NetInit();                                                      /*  ��ʼ������ϵͳ              */

    /*
     *  ��ʼ�����總�ӹ���
     */
#if LW_CFG_NET_PING_EN > 0
    API_INetPingInit();
    API_INetPing6Init();
#endif                                                                  /*  LW_CFG_NET_PING_EN > 0      */

#if LW_CFG_NET_NETBIOS_EN > 0
    API_INetNetBiosInit();
    API_INetNetBiosNameSet("sylixos");
#endif                                                                  /*  LW_CFG_NET_NETBIOS_EN > 0   */

#if LW_CFG_NET_TFTP_EN > 0
    API_INetTftpServerInit("/tmp");
#endif                                                                  /*  LW_CFG_NET_TFTP_EN > 0      */

#if LW_CFG_NET_FTPD_EN > 0
    API_INetFtpServerInit("/");
#endif                                                                  /*  LW_CFG_NET_FTP_EN > 0       */

#if LW_CFG_NET_TELNET_EN > 0
    API_INetTelnetInit(LW_NULL);
#endif                                                                  /*  LW_CFG_NET_TELNET_EN > 0    */

#if LW_CFG_NET_NAT_EN > 0
    API_INetNatInit();
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */

#if LW_CFG_NET_NPF_EN > 0
    API_INetNpfInit();
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */

#if LW_CFG_NET_VPN_EN > 0
    API_INetVpnInit();
#endif                                                                  /*  LW_CFG_NET_VPN_EN > 0       */
}
/*********************************************************************************************************
  designware ��̫��ƽ̨����
*********************************************************************************************************/
static DESIGNWARE_ETH_DATA  _G_designwareEthData[2] = {
        {
            .ulBase    = MIPS32_KSEG1_PA(BSP_CFG_GMAC0_BASE),
            .ulVector  = BSP_CFG_INT1_SUB_VECTOR(2),
            .uiPhyId   = BSP_CFG_GMAC0_PHY_ID,
            .ucMacAddr = { 0x0, 0x55, 0x7b, 0xb5, 0x7d, 0xf7 },
        },
        {
            .ulBase    = MIPS32_KSEG1_PA(BSP_CFG_GMAC1_BASE),
            .ulVector  = BSP_CFG_INT1_SUB_VECTOR(3),
            .uiPhyId   = BSP_CFG_GMAC1_PHY_ID,
            .ucMacAddr = { 0x0, 0x55, 0x7b, 0xb5, 0x7d, 0xf8 },
        }
};
/*********************************************************************************************************
** ��������: halNetifAttch
** ��������: ����ӿ�����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halNetifAttch (VOID)
{
    static struct netif  ethernetif[2];
    ip_addr_t            ip, submask, gateway;
    struct netif        *pnetif;

    /*
     * ����һ
     */
    pnetif = &ethernetif[0];

    IP4_ADDR(&ip,       192, 168,   1,  55);
    IP4_ADDR(&submask,  255, 255, 255,   0);
    IP4_ADDR(&gateway,  192, 168,   1,   1);

    netif_add(pnetif, &ip, &submask, &gateway,
              &_G_designwareEthData[0], designwareEthInit,
              tcpip_input);

    pnetif->ip6_autoconfig_enabled = 1;                                 /*  ���� IPv6 ��ַ�Զ�����      */

    netif_set_up(pnetif);

    netif_set_default(pnetif);

    netif_create_ip6_linklocal_address(pnetif, 1);                      /*  ���� ipv6 link address      */

    lib_srand((pnetif->hwaddr[0] << 24) |
              (pnetif->hwaddr[1] << 16) |
              (pnetif->hwaddr[2] <<  8) |
              (pnetif->hwaddr[3]));                                     /*  ������ mac �������������   */

    /*
     * ���ڶ�
     */
#if 0
    pnetif = &ethernetif[1];

    IP4_ADDR(&ip,       192, 168,   3,  11);
    IP4_ADDR(&submask,  255, 255, 255,   0);
    IP4_ADDR(&gateway,  192, 168,   3,   1);

    netif_add(pnetif, &ip, &submask, &gateway,
              &_G_designwareEthData[1], designwareEthInit,
              tcpip_input);

    pnetif->ip6_autoconfig_enabled = 1;                                 /*  ���� IPv6 ��ַ�Զ�����      */

    netif_set_up(pnetif);

    netif_create_ip6_linklocal_address(pnetif, 1);                      /*  ���� ipv6 link address      */
#endif                                                                  /*  �ο��������                */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** ��������: halMonitorInit
** ��������: �ں˼�����ϴ���ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

static VOID  halMonitorInit (VOID)
{
    /*
     *  ���������ﴴ���ں˼�����ϴ�ͨ��, Ҳ����ʹ�� shell �������.
     */
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
** ��������: halPosixInit
** ��������: ��ʼ�� posix ��ϵͳ (���ϵͳ֧�� proc �ļ�ϵͳ, �������� proc �ļ�ϵͳ��װ֮��!)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

static VOID  halPosixInit (VOID)
{
    API_PosixInit();
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** ��������: halSymbolInit
** ��������: ��ʼ��Ŀ��ϵͳ���ű���, (Ϊ module loader �ṩ����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0

static VOID  halSymbolInit (VOID)
{
#ifdef __GNUC__
    void *__aeabi_read_tp();
#endif                                                                  /*  __GNUC__                    */

    API_SymbolInit();

#ifdef __GNUC__
    symbolAddAll();

    /*
     *  GCC will emit calls to this routine under -mtp=soft.
     */
    API_SymbolAdd("__aeabi_read_tp", (caddr_t)__aeabi_read_tp, LW_SYMBOL_FLAG_XEN);
#endif                                                                  /*  __GNUC__                    */
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** ��������: halLoaderInit
** ��������: ��ʼ��Ŀ��ϵͳ�����ģ��װ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  halLoaderInit (VOID)
{
    API_LoaderInit();
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** ��������: halStdDirInit
** ��������: �豸��׼�ļ�ϵͳ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halStdDirInit (VOID)
{
    /*
     *  �� LW_CFG_PATH_VXWORKS == 0 ʱ, ����ϵͳ���ڸ�Ŀ¼���Զ������������ص� /dev /mnt /var
     *  ������ص���Ҫͨ���ֶ�����, ����: /etc /bin /sbin /tmp �ȵ�,
     *  һ����˵ /tmp /bin /sbin /ftk /etc ... �������������ļ�, ���ӵ�ָ�����ļ�ϵͳ.
     */
    mkdir("/usb", DEFAULT_DIR_PERM);
    if (access("/yaffs2/n0/boot", R_OK) < 0) {
        mkdir("/yaffs2/n0/boot", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n0/etc", R_OK) < 0) {
        mkdir("/yaffs2/n0/etc", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/ftk", R_OK) < 0) {
        mkdir("/yaffs2/n1/ftk", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/qt", R_OK) < 0) {
        mkdir("/yaffs2/n1/qt", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/lib", R_OK) < 0) {
        mkdir("/yaffs2/n1/lib", DEFAULT_DIR_PERM);
        mkdir("/yaffs2/n1/lib/modules", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/usr", R_OK) < 0) {
        mkdir("/yaffs2/n1/usr", DEFAULT_DIR_PERM);
        mkdir("/yaffs2/n1/usr/lib", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/bin", R_OK) < 0) {
        mkdir("/yaffs2/n1/bin", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/tmp", R_OK) < 0) {
        mkdir("/yaffs2/n1/tmp", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/sbin", R_OK) < 0) {
        mkdir("/yaffs2/n1/sbin", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/apps", R_OK) < 0) {
        mkdir("/yaffs2/n1/apps", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/home", R_OK) < 0) {
        mkdir("/yaffs2/n1/home", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/root", R_OK) < 0) {
        mkdir("/yaffs2/n1/root", DEFAULT_DIR_PERM);
    }
    if (access("/yaffs2/n1/var", R_OK) < 0) {
        mkdir("/yaffs2/n1/var", DEFAULT_DIR_PERM);
    }

    symlink("/yaffs2/n0/boot", "/boot");
    symlink("/yaffs2/n0/etc",  "/etc");                                 /*  ������Ŀ¼��������          */
    symlink("/yaffs2/n1/ftk",  "/ftk");                                 /*  ���� FTK ͼ��ϵͳ��������   */
    symlink("/yaffs2/n1/qt",   "/qt");                                  /*  ���� Qt ͼ��ϵͳ��������    */
    symlink("/yaffs2/n1/lib",  "/lib");
    symlink("/yaffs2/n1/usr",  "/usr");
    symlink("/yaffs2/n1/bin",  "/bin");
    symlink("/yaffs2/n1/sbin", "/sbin");
    symlink("/yaffs2/n1/apps", "/apps");
    symlink("/yaffs2/n1/home", "/home");
    symlink("/yaffs2/n1/root", "/root");
    symlink("/yaffs2/n1/var",  "/var");
    symlink("/yaffs2/n1/tmp",  "/var/tmp");
    symlink("/yaffs2/n1/tmp",  "/tmp");
}
/*********************************************************************************************************
** ��������: halBootThread
** ��������: ������״̬�µĳ�ʼ����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  halBootThread (PVOID  pvBootArg)
{
    LW_CLASS_THREADATTR     threakattr = API_ThreadAttrGetDefault();    /*  ʹ��Ĭ������                */

    (VOID)pvBootArg;

#if LW_CFG_SHELL_EN > 0
    halShellInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_POWERM_EN > 0
    halPmInit();
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

#if LW_CFG_DEVICE_EN > 0
    ls1xClockInit();                                                    /*  Clock Init                  */
    halBusInit();
    halDrvInit();
    halDevInit();
    halStdFileInit();
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

#if LW_CFG_LOG_LIB_EN > 0
    halLogInit();
    console_loglevel = default_message_loglevel;                        /*  ���� printk ��ӡ��Ϣ�ȼ�    */
#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */

    /*
     *  ��Ϊ yaffs ���������ʱ, ��Ҫ stdout ��ӡ��Ϣ, ����� halDevInit() �б�����, ����û�д���
     *  ��׼�ļ�, ���Ի��ӡ���������Ϣ, ���Խ��˺�����������!
     *  ���δ��ʼ����׼�ļ�����ʾ������Ϣ
     */

#ifdef __GNUC__
    nand_init();
    mtdDevCreateEx("/n");                                               /*  mount mtddevice             */
#else
    nandDevCreateEx("/n");                                              /*  mount nandflash disk(yaffs) */
#endif

    halStdDirInit();                                                    /*  ������׼Ŀ¼                */

    /*
     *  ֻ�г�ʼ���� shell ������� TZ ����������ʾ��ʱ��, �ſ��Ե��� rtcToRoot()
     */
    system("varload");                                                  /*  ��/etc/profile�ж�ȡ��������*/
    lib_tzset();                                                        /*  ͨ�� TZ ������������ʱ��    */
    rtcToRoot();                                                        /*  �� RTC ʱ��ͬ�������ļ�ϵͳ */

    /*
     *  �����ʼ��һ����� shell ��ʼ��֮��, ��Ϊ��ʼ���������ʱ, ���Զ�ע�� shell ����.
     */
#if LW_CFG_NET_EN > 0
    halNetInit();
    halNetifAttch();                                                    /*  wlan ������Ҫ���ع̼�       */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

#if LW_CFG_POSIX_EN > 0
    halPosixInit();
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#if LW_CFG_SYMBOL_EN > 0
    halSymbolInit();
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */

#if LW_CFG_MODULELOADER_EN > 0
    halLoaderInit();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_MONITOR_EN > 0
    halMonitorInit();
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

    system("shfile /yaffs2/n0/etc/startup.sh");                         /*  ִ�������ű�                */
                                                                        /*  �����ڳ�ʼ�� shell �����!! */

    API_ThreadAttrSetStackSize(&threakattr, __LW_THREAD_MAIN_STK_SIZE); /*  ���� main �̵߳Ķ�ջ��С    */
    API_ThreadCreate("t_main",
                     (PTHREAD_START_ROUTINE)t_main,
                     &threakattr,
                     LW_NULL);                                          /*  Create "t_main()" thread    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: usrStartup
** ��������: ��ʼ��Ӧ��������, ��������ϵͳ�ĵ�һ������.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  usrStartup (VOID)
{
    LW_CLASS_THREADATTR     threakattr;

    /*
     *  ע��, ��Ҫ�޸ĸó�ʼ��˳�� (�����ȳ�ʼ�� vmm ������ȷ�ĳ�ʼ�� cache,
     *                              ������Ҫ������Դ��������ʼ��)
     */
    halIdleInit();
#if LW_CFG_CPU_FPU_EN > 0
    halFpuInit();
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_RTC_EN > 0
    halTimeInit();
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */

#if LW_CFG_VMM_EN > 0
    halVmmInit();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#if LW_CFG_CACHE_EN > 0
    halCacheInit();
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    API_ThreadAttrBuild(&threakattr,
                        __LW_THREAD_BOOT_STK_SIZE,
                        LW_PRIO_CRITICAL,
                        LW_OPTION_THREAD_STK_CHK,
                        LW_NULL);
    API_ThreadCreate("t_boot",
                     (PTHREAD_START_ROUTINE)halBootThread,
                     &threakattr,
                     LW_NULL);                                          /*  Create boot thread          */
}
/*********************************************************************************************************
** ��������: main
** ��������: C ���
** �䡡��  : NONE
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT bspInit (VOID)
{
    /*
     *  ϵͳ�ں˶���ϵͳ��
     */
    static __section(.noinit) CHAR  cKernelHeap[12 * LW_CFG_MB_SIZE];

    halModeInit();                                                      /*  ��ʼ��Ӳ��                  */

    /*
     *  ����ĵ��Զ˿����������ϵͳ��, ������Ӧ�ò������ڲ���ϵͳ������.
     *  ��ϵͳ���ִ���ʱ, ����˿��Ե���Ϊ�ؼ�. (��Ŀ��������ͨ�����ùص�)
     */
#if 0                                                                   /*  �ο����뿪ʼ                */
    debugChannelInit(0);                                                /*  ��ʼ�����Խӿ�              */
#endif                                                                  /*  �ο��������                */

    /*
     *  ����ʹ�� bsp ������������, ��� bootloader ֧��, ��ʹ�� bootloader ����.
     *  Ϊ�˼�����ǰ����Ŀ, ���� kfpu=yes �����ں���(�����ж�)ʹ�� FPU.
     *
     *  TODO: �����޸��ں���������
     */
    API_KernelStartParam("ncpus=1 kdlog=no kderror=yes kfpu=no heapchk=yes hz=100"
                         "varea=0xc0000000 vsize=0x3fffe000");
                                                                        /*  ����ϵͳ������������        */
    API_KernelStart(usrStartup,
                    cKernelHeap,
                    sizeof(cKernelHeap),
                    LW_NULL,
                    0);                                                 /*  �����ں�                    */

    return  (0);                                                        /*  ����ִ�е�����              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
