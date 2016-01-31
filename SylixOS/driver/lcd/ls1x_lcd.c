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
** ��   ��   ��: ls1x_lcd.c
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
#include "../clock/ls1x_clock.h"
#include <linux/compat.h>
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define LCD_BASE_ADDR0          (0xBC301240)
#define LCD_BASE_ADDR1          (0xBC301250)
#define LCD_XSIZE               (480)
#define LCD_YSIZE               (272)
#define LS1X_CLK_PLL_DIV        (0xBFE78034)
#define LCD_BITS_PER_PIXEL      (16)
#define LCD_BYTES_PER_PIXEL     (2)
#define LCD_RED_MASK            (0xF800)
#define LCD_GREEN_MASK          (0x03E0)
#define LCD_BLUE_MASK           (0x001F)
#define BURST_SIZE              (0x7F)
/*********************************************************************************************************
  LCD ���������Ͷ���
*********************************************************************************************************/
typedef struct {
    LW_GM_DEVICE                LCDC_gmDev;                             /*  ͼ���豸                    */
    CPCHAR                      LCDC_pcName;                            /*  �豸��                      */
    UINT                        LCDD_iLcdMode;                          /*  �豸MOde                    */
    PVOID                       LCDC_pvFrameBuffer;
} LS1XLCD_CONTROLER, *PLS1XLCD_CONTROLER;
/*********************************************************************************************************
  ��ʾ��Ϣ
*********************************************************************************************************/
static  LS1XLCD_CONTROLER       _G_ls1xLcdControler;
static  LW_GM_FILEOPERATIONS    _G_ls1xLcdGmFileOper;

static struct lcd_struc {
    UINT uiPclk, uiRefresh;
    UINT uiHR, uiHSS, uiHSE, uiHFL;
    UINT uiVR, uiVSS, uiVSE, uiVFL;
    UINT uiPanConfig;                                                   /* ��ͬ��lcd����ֵ���ܲ�ͬ    */
    UINT uiHVSyncPolarity;                                              /* ��ͬ��lcd����ֵ���ܲ�ͬ    */
} lcdmode[] = {
        /*"480x272_60.00"*/
        {9009, 60, 480, 488, 489, 531, 272, 276, 282, 288, 0x00000101, 0xC0000000},
};

enum {
    OF_BUF_CONFIG               = 0,
    OF_BUF_ADDR0                = 0x20,
    OF_BUF_STRIDE               = 0x40,
    OF_BUF_ORIG                 = 0x60,
    OF_DITHER_CONFIG            = 0x120,
    OF_DITHER_TABLE_LOW         = 0x140,
    OF_DITHER_TABLE_HIGH        = 0x160,
    OF_PAN_CONFIG               = 0x180,
    OF_PAN_TIMING               = 0x1A0,
    OF_HDISPLAY                 = 0x1C0,
    OF_HSYNC                    = 0x1E0,
    OF_VDISPLAY                 = 0x240,
    OF_VSYNC                    = 0x260,
    OF_BUF_ADDR1                = 0x340,
};

/*********************************************************************************************************
** ��������: ls1xconfigFb
** ��������: ����LCD Control��Ӧ�Ĳ���
** �䡡��  : uiBase  LCD Reg Base Address
** �䡡��  : �������.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT ls1xconfigFb(UINT32 uiBase)
{
    INT     iIndex, iLcdMode;
    UINT32  uiDivider, uiDivReg;
    UINT32  uiVal;
    INT     iTimeout = 204800;

    for (iIndex = 0; iIndex < sizeof(lcdmode) / sizeof(struct lcd_struc); iIndex++) {
        if (lcdmode[iIndex].uiHR == LCD_XSIZE && lcdmode[iIndex].uiVR == LCD_YSIZE) {
            iLcdMode = iIndex;

            uiDivider = ls1xPLLClock() / (lcdmode[iLcdMode].uiPclk * 1000) / 4;
            /* ����Ƶ��ֵ*/
            if (uiDivider < 1) {
                uiDivider = 1;
            }
            else if (uiDivider > 15) {
                uiDivider = 15;
            }

            /* ���÷�Ƶ�Ĵ��� */
            uiDivReg  = readl(LS1X_CLK_PLL_DIV);
            uiDivReg |= 0x00003000;                                     /*  dc_bypass ��1               */
            uiDivReg &= ~0x00000030;                                    /*  dc_rst ��0                  */
            uiDivReg &= ~(0x1f << 26);                                  /*  dc_div ����                 */
            uiDivReg |= (uiDivider << 26);
            writel(uiDivReg, LS1X_CLK_PLL_DIV);
            uiDivReg &= ~0x00001000;                                    /*  dc_bypass ��0               */
            writel(uiDivReg, LS1X_CLK_PLL_DIV);
            break;
        }
    }

    if (iLcdMode < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Unsupported framebuffer resolution,choose from bellow.\r\n");
        return  (PX_ERROR);
    }

    /* ��ֹPanle 0 */
    writel((UINT32)_G_ls1xLcdControler.LCDC_pvFrameBuffer, uiBase + OF_BUF_ADDR0);
    writel((UINT32)_G_ls1xLcdControler.LCDC_pvFrameBuffer, uiBase + OF_BUF_ADDR1);

    /* PAN_CONFIG�Ĵ������λ��Ҫ��1,����lcdʱ����ʱһ��ʱ��Ż������ */
    writel(0x80001111 | lcdmode[iLcdMode].uiPanConfig, uiBase + OF_PAN_CONFIG);

    writel((lcdmode[iLcdMode].uiHFL << 16) | lcdmode[iLcdMode].uiHR, uiBase + OF_HDISPLAY);
    writel( lcdmode[iLcdMode].uiHVSyncPolarity | (lcdmode[iLcdMode].uiHSE << 16) |
            lcdmode[iLcdMode].uiHSS, uiBase + OF_HSYNC);
    writel((lcdmode[iLcdMode].uiVFL << 16) | lcdmode[iLcdMode].uiVR, uiBase + OF_VDISPLAY);
    writel( lcdmode[iLcdMode].uiHVSyncPolarity | (lcdmode[iLcdMode].uiVSE << 16) |
            lcdmode[iLcdMode].uiVSS, uiBase + OF_VSYNC);

    switch (_G_ls1xLcdControler.LCDD_iLcdMode) {
    case 32:
        writel(0x00100104, uiBase + OF_BUF_CONFIG);
        writel((LCD_XSIZE * 4 + BURST_SIZE) & ~BURST_SIZE, uiBase + OF_BUF_STRIDE);
#ifdef LS1BSOC
        writel(0xBFD00420, (readl(0xBFD00420) & ~0x08));
        writel(0xBFD00420, (readl(0xBFD00420) |  0x05));
#endif
        break;

    case 24:
        writel(0x00100104, uiBase + OF_BUF_CONFIG);
        writel((LCD_XSIZE * 3 + BURST_SIZE) & ~BURST_SIZE, uiBase + OF_BUF_STRIDE);
#ifdef LS1BSOC
        writel(0xBFD00420, (readl(0xBFD00420) & ~0x08));
        writel(0xBFD00420, (readl(0xBFD00420) |  0x05));
#endif
        break;

    case 16:
    default:
        writel(0x00100103, uiBase + OF_BUF_CONFIG);
        writel((LCD_XSIZE * 2 + BURST_SIZE) & ~BURST_SIZE, uiBase + OF_BUF_STRIDE);
#ifdef LS1BSOC
        writel(0xBFD00420, (readl(0xBFD00420) & ~0x07));
#endif
        break;

    case 15:
        writel(0x00100102, uiBase + OF_BUF_CONFIG);
        writel((LCD_XSIZE * 2 + BURST_SIZE) & ~BURST_SIZE, uiBase + OF_BUF_STRIDE);
#ifdef LS1BSOC
        writel(0xBFD00420, (readl(0xBFD00420) & ~0x07));
#endif
        break;

    case 12:
        writel(0x00100101, uiBase + OF_BUF_CONFIG);
        writel((LCD_XSIZE * 2 + BURST_SIZE) & ~BURST_SIZE, uiBase + OF_BUF_STRIDE);
#ifdef LS1BSOC
        writel(0xBFD00420, (readl(0xBFD00420) & ~0x07));
#endif
        break;
    }

    writel(0, uiBase + OF_BUF_ORIG);
    /* ��ʾ�������ʹ�� */
    uiVal = readl((uiBase + OF_BUF_CONFIG));
    do {
        writel(uiVal | 0x100, uiBase + OF_BUF_CONFIG);
        uiVal = readl(uiBase + OF_BUF_CONFIG);
    } while (((uiVal & 0x100) == 0) && (iTimeout-- > 0));

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: ls1xLcdOpen
** ��������: �� FB
** �䡡��  : pgmdev    fb �豸
**           iFlag     ���豸��־
**           iMode     ���豸ģʽ
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xLcdOpen (PLW_GM_DEVICE  pgmdev, INT  iFlag, INT  iMode)
{
    PLS1XLCD_CONTROLER      pControler  = &_G_ls1xLcdControler;

    if (!pControler->LCDC_pvFrameBuffer) {
        pControler->LCDC_pvFrameBuffer = API_VmmPhyAllocEx(LCD_XSIZE * LCD_YSIZE * LCD_BYTES_PER_PIXEL,
                                                           LW_ZONE_ATTR_DMA);
        if (pControler->LCDC_pvFrameBuffer == LW_NULL) {
            return  (PX_ERROR);
        }
    }

    ls1xconfigFb(LCD_BASE_ADDR0);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xLcdClose
** ��������: FB �رպ���
** �䡡��  : pgmdev    fb �豸
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ls1xLcdClose (PLW_GM_DEVICE  pgmdev)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xLcdGetVarInfo
** ��������: ��� FB ��Ϣ
** �䡡��  : pgmdev    fb �豸
**           pgmsi     fb ��Ļ��Ϣ
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT      ls1xLcdGetVarInfo (PLW_GM_DEVICE  pgmdev, PLW_GM_VARINFO  pgmvi)
{
    if (pgmvi) {
        pgmvi->GMVI_ulXRes              = LCD_XSIZE;
        pgmvi->GMVI_ulYRes              = LCD_YSIZE;

        pgmvi->GMVI_ulXResVirtual       = LCD_XSIZE;
        pgmvi->GMVI_ulYResVirtual       = LCD_YSIZE;

        pgmvi->GMVI_ulXOffset           = 0;
        pgmvi->GMVI_ulYOffset           = 0;

        pgmvi->GMVI_ulBitsPerPixel      = LCD_BITS_PER_PIXEL;
        pgmvi->GMVI_ulBytesPerPixel     = LCD_BYTES_PER_PIXEL;

        pgmvi->GMVI_ulGrayscale         = (1 << LCD_BITS_PER_PIXEL);

        /*
         *  RGB565 RED 0xF800, GREEN 0x07E0, BLUE 0x001F
         */
        pgmvi->GMVI_ulRedMask           = LCD_RED_MASK;
        pgmvi->GMVI_ulGreenMask         = LCD_GREEN_MASK;
        pgmvi->GMVI_ulBlueMask          = LCD_BLUE_MASK;
        pgmvi->GMVI_ulTransMask         = 0;

        pgmvi->GMVI_bHardwareAccelerate = LW_FALSE;
        pgmvi->GMVI_ulMode              = LW_GM_SET_MODE;
        pgmvi->GMVI_ulStatus            = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1xLcdGetScrInfo
** ��������: ��� LCD ��Ļ��Ϣ
** �䡡��  : pgmdev    fb �豸
**           pgmsi     fb ��Ļ��Ϣ
** �䡡��  : �����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT      ls1xLcdGetScrInfo (PLW_GM_DEVICE  pgmdev, PLW_GM_SCRINFO  pgmsi)
{
    PLS1XLCD_CONTROLER      pControler  = &_G_ls1xLcdControler;

    if (pgmsi) {
        pgmsi->GMSI_pcName           = (PCHAR)pControler->LCDC_pcName;
        pgmsi->GMSI_ulId             = 0;
        pgmsi->GMSI_stMemSize        = LCD_XSIZE * LCD_YSIZE * LCD_BYTES_PER_PIXEL;
        pgmsi->GMSI_stMemSizePerLine = LCD_XSIZE * LCD_BYTES_PER_PIXEL;
        pgmsi->GMSI_pcMem            = (caddr_t)pControler->LCDC_pvFrameBuffer;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: ls1bFbDevCreate
** ��������: ���� FrameBuffer �豸
** �䡡��  : cpcName          �豸��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  ls1bFbDevCreate(CPCHAR  cpcName)
{
    PLS1XLCD_CONTROLER      pControler  = &_G_ls1xLcdControler;
    PLW_GM_FILEOPERATIONS   pGmFileOper = &_G_ls1xLcdGmFileOper;

    if (cpcName == LW_NULL) {
        return  (PX_ERROR);
    }

    /*
     *  ��֧�� framebuffer ģʽ.
     */
    pGmFileOper->GMFO_pfuncOpen         = ls1xLcdOpen;
    pGmFileOper->GMFO_pfuncClose        = ls1xLcdClose;
    pGmFileOper->GMFO_pfuncGetVarInfo   = (INT (*)(LONG, PLW_GM_VARINFO))ls1xLcdGetVarInfo;
    pGmFileOper->GMFO_pfuncGetScrInfo   = (INT (*)(LONG, PLW_GM_SCRINFO))ls1xLcdGetScrInfo;

    pControler->LCDC_gmDev.GMDEV_gmfileop   = pGmFileOper;
    pControler->LCDC_gmDev.GMDEV_ulMapFlags = LW_VMM_FLAG_DMA | LW_VMM_FLAG_BUFFERABLE;
    pControler->LCDC_pcName                 = cpcName;
    pControler->LCDD_iLcdMode               = LCD_BITS_PER_PIXEL;

    return  (gmemDevAdd(pControler->LCDC_pcName, &pControler->LCDC_gmDev));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

