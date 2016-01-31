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
** 文   件   名: ls1x_wdt.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 12 月 13 日
**
** 描        述: loongson1B NAND Flash 驱动文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#define  __raw_writel writel
#define  __raw_readl  readl
#undef   ALIGN
#include "ls1x_nand.h"

static UINT order_addr_in;

struct ls1x_nand_info {
    struct nand_chip            nand_chip;                              /*  MTD data control            */

    UINT                        buf_start;
    UINT                        buf_count;

    UINT                        mmio_base;                              /*  NAND registers              */

    UINT                        dma_desc;
    UINT                        dma_desc_phys;
    size_t                      dma_desc_size;

    UCHAR                      *data_buff;
    UINT                        data_buff_phys;
    size_t                      data_buff_size;

    UINT                        cmd;

    UINT                        seqin_column;
    UINT                        seqin_page_addr;
};

static struct                   mtd_info *ls1x_mtd = NULL;

/*********************************************************************************************************
** 函数名称: nand_gpio_init
** 功能描述: NAND GPIO Init
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID nand_gpio_init(VOID)
{
#ifdef LS1ASOC
{
    int val;
#ifdef NAND_USE_LPC                                                     /*  NAND复用LPC PWM01           */
    val = __raw_readl(GPIO_MUX);
    val |= 0x2a000000;
    __raw_writel(val, GPIO_MUX);

    val = __raw_readl(GPIO_CONF2);
    val &= ~(0xffff<<6);                                                /*  nand_D0~D7 & nand_control pin*/
    __raw_writel(val, GPIO_CONF2);
#elif NAND_USE_SPI1                                                     /*  NAND复用SPI1 PWM23          */
    val = __raw_readl(GPIO_MUX);
    val |= 0x14000000;
    __raw_writel(val, GPIO_MUX);

    val = __raw_readl(GPIO_CONF1);
    val &= ~(0xf<<12);                                                  /*  nand_D0~D3                  */
    __raw_writel(val, GPIO_CONF1);

    val = __raw_readl(GPIO_CONF2);
    val &= ~(0xfff<<12);                                                /*  nand_D4~D7 & nand_control pin*/
    __raw_writel(val, GPIO_CONF2);
#endif
}
#endif
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_waitfunc
** 功能描述: 等待NAND 芯片完成命令
** 输　入  : mtd_info
**         : nand_chip
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT ls1x_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
    return 0;
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_select_chip
** 功能描述: NAND 芯片的选择，对于多颗芯片
** 输　入  : mtd_info
**         : nand_chip
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID ls1x_nand_select_chip(struct mtd_info *mtd, INT chip)
{
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_dev_ready
** 功能描述: NAND 芯片ready信号判断
** 输　入  : mtd_info
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT ls1x_nand_dev_ready(struct mtd_info *mtd)
{
    /*
     * 多片flash的rdy信号如何判断
     */
    UINT    ret;
    struct  ls1x_nand_info *info = mtd->priv;

    ret = nand_readl(info, NAND_CMD) & 0x000f0000;
    if (ret) {
        return 1;
    }
    return 0;
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_read_buf
** 功能描述: NAND 芯片read Data to Bufffer
** 输　入  : mtd_info
**         : pbuf
**         : iLen
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID ls1x_nand_read_buf(struct mtd_info *mtd, UINT8 *pbuf, INT iLen)
{
    struct ls1x_nand_info *info = mtd->priv;
    INT iRealLen = min_t(size_t, iLen, info->buf_count - info->buf_start);

    memcpy(pbuf, info->data_buff + info->buf_start, iRealLen);
    info->buf_start += iRealLen;
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_read_word
** 功能描述: 从NAND 芯片读取一个字
** 输　入  : mtd_info
** 输　出  : Len
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT16 ls1x_nand_read_word(struct mtd_info *mtd)
{
    struct  ls1x_nand_info *info = mtd->priv;
    UINT16  uiRetval = 0xFFFF;

    if (!(info->buf_start & 0x1) && info->buf_start < info->buf_count) {
        uiRetval = *(UINT16 *)(info->data_buff + info->buf_start);
    }

    info->buf_start += 2;

    return uiRetval;
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_read_byte
** 功能描述: 从NAND 芯片读取一个字节
** 输　入  : mtd_info
** 输　出  : Len
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8 ls1x_nand_read_byte(struct mtd_info *mtd)
{
    struct  ls1x_nand_info *info = mtd->priv;
    CHAR    Retval = 0xFF;

    if (info->buf_start < info->buf_count)
        Retval = info->data_buff[(info->buf_start)++];

    return Retval;
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_write_buf
** 功能描述: NAND 芯片Write Data to Bufffer
** 输　入  : mtd_info
**         : pbuf
**         : iLen
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID ls1x_nand_write_buf(struct mtd_info *mtd, const UINT8 *pbuf, INT iLen)
{
    struct ls1x_nand_info *info = mtd->priv;
    INT iRealLen = min_t(size_t, iLen, info->buf_count - info->buf_start);

    memcpy(info->data_buff + info->buf_start, pbuf, iRealLen);
    info->buf_start += iRealLen;
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_verify_buf
** 功能描述: 从NAND 芯片读取 Data 并验证
** 输　入  : mtd_info
**         : pbuf
**         : iLen
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int ls1x_nand_verify_buf(struct mtd_info *mtd, const UINT8 *buf, INT iLen)
{
    int i = 0;

    while (iLen--) {
        if (buf[i++] != ls1x_nand_read_byte(mtd) ) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "ls1x_nand_verify_buf verify error!\r\n");
            return (-1);
        }
    }

    return (0);
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_done
** 功能描述: 判断操作是否完成
** 输　入  : ls1x_nand_info
** 输　出  : Len
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT ls1x_nand_done(struct ls1x_nand_info *info)
{
    INT iRet, iTimeout = 40000;

    do {
        iRet = nand_readl(info, NAND_CMD);
        iTimeout--;
    } while (((iRet & 0x400) != 0x400) && iTimeout);

    return iTimeout;
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_start
** 功能描述: 操作开始
** 输　入  : ls1x_nand_info
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID inline ls1x_nand_start(struct ls1x_nand_info *info)
{
    nand_writel(info, NAND_CMD, nand_readl(info, NAND_CMD) | 0x1);
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_stop
** 功能描述: 操作停止
** 输　入  : ls1x_nand_info
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID inline ls1x_nand_stop(struct ls1x_nand_info *info)
{
}
/*********************************************************************************************************
** 函数名称: start_dma_nand
** 功能描述: DAM 操作启动
** 输　入  : flags
**         : ls1x_nand_info
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID start_dma_nand(UINT flags, struct ls1x_nand_info *info)
{
    INT     iTimeout = 5000;
    int     iRet;

    writel(0, info->dma_desc + DMA_ORDERED);
    writel(info->data_buff_phys, info->dma_desc + DMA_SADDR);
    writel(DMA_ACCESS_ADDR, info->dma_desc + DMA_DADDR);
    writel(((info->buf_count + 3) >> 2), info->dma_desc + DMA_LENGTH);
    writel(0, info->dma_desc + DMA_STEP_LENGTH);
    writel(1, info->dma_desc + DMA_STEP_TIMES);

    if (flags) {
        writel(0x00003001, info->dma_desc + DMA_CMD);
    } else {
        writel(0x00000001, info->dma_desc + DMA_CMD);
    }

    ls1x_nand_start(info);                                              /*  使能nand命令                */
    writel((info->dma_desc_phys & ~0x1F) | 0x8, order_addr_in);         /*  启动DMA                     */
    while ((readl(order_addr_in) & 0x8)) {
    }

    /*
     * K9F5608U0D在读的时候 ls1x的nand flash控制器读取不到完成状态
     * 猜测是控制器对该型号flash兼容不好,目前解决办法是添加一段延时
     */
#ifdef K9F5608U0D
    if (flags) {
        if (!ls1x_nand_done(info)) {
            printf("Wait time out!!!\n");
            ls1x_nand_stop(info);
        }
    } else {
        udelay(50);
    }
#else
    if (!ls1x_nand_done(info)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Wait time out!\r\n");
        ls1x_nand_stop(info);
    }
#endif

    while (iTimeout) {
        writel((info->dma_desc_phys & (~0x1F)) | 0x4, order_addr_in);
        do {
        } while (readl(order_addr_in) & 0x4);
        iRet = readl(info->dma_desc + DMA_CMD);
        if (iRet & 0x08) {
            break;
        }
        iTimeout--;
    }

    if (!iTimeout) {
    }
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_cmdfunc
** 功能描述: 向NAND 芯片发送命令
** 输　入  : mtd_info
**         : command
**         : column
**         : page_addr
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID ls1x_nand_cmdfunc(struct mtd_info *mtd, UINT command, INT column, INT page_addr)
{
    struct ls1x_nand_info *info = mtd->priv;

    switch (command) {
    case NAND_CMD_READOOB:
        info->buf_count = mtd->oobsize;
        info->buf_start = column;
        nand_writel(info, NAND_CMD, SPARE | READ);
        nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr) + mtd->writesize);
        nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
        nand_writel(info, NAND_OPNUM, info->buf_count);
        nand_writel(info, NAND_PARAM,
                (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16));
        start_dma_nand(0, info);
        break;

    case NAND_CMD_READ0:
        info->buf_count = mtd->writesize + mtd->oobsize;
        info->buf_start = column;
        nand_writel(info, NAND_CMD, SPARE | MAIN | READ);
        nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr));
        nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
        nand_writel(info, NAND_OPNUM, info->buf_count);
        nand_writel(info, NAND_PARAM,
                (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
        start_dma_nand(0, info);
        break;

    case NAND_CMD_SEQIN:
        info->buf_count = mtd->writesize + mtd->oobsize - column;
        info->buf_start = 0;
        info->seqin_column = column;
        info->seqin_page_addr = page_addr;
        break;

    case NAND_CMD_PAGEPROG:
        if (info->seqin_column < mtd->writesize)
            nand_writel(info, NAND_CMD, SPARE | MAIN | WRITE);
        else
            nand_writel(info, NAND_CMD, SPARE | WRITE);
        nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(info->seqin_page_addr) + info->seqin_column);
        nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(info->seqin_page_addr));
        nand_writel(info, NAND_OPNUM, info->buf_count);
        nand_writel(info, NAND_PARAM,
                (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
        start_dma_nand(1, info);
        break;

    case NAND_CMD_RESET:
        info->buf_count = 0x0;
        info->buf_start = 0x0;
        nand_writel(info, NAND_CMD, RESET);
        ls1x_nand_start(info);
        if (!ls1x_nand_done(info)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Wait time out!\r\n");
            ls1x_nand_stop(info);
        }
        break;

    case NAND_CMD_ERASE1:
        info->buf_count = 0x0;
        info->buf_start = 0x0;
        nand_writel(info, NAND_ADDR_L, MAIN_ADDRL(page_addr));
        nand_writel(info, NAND_ADDR_H, MAIN_ADDRH(page_addr));
        nand_writel(info, NAND_OPNUM, 0x01);
        nand_writel(info, NAND_PARAM,
                (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (0x1 << 16)); /* 1C注意 */
        nand_writel(info, NAND_CMD, ERASE);
        ls1x_nand_start(info);
        if (!ls1x_nand_done(info)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Wait time out!\r\n");
            ls1x_nand_stop(info);
        }
        break;

    case NAND_CMD_STATUS:
        info->buf_count = 0x1;
        info->buf_start = 0x0;
        nand_writel(info, NAND_CMD, READ_STATUS);
        ls1x_nand_start(info);
        if (!ls1x_nand_done(info)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Wait time out!\r\n");
            ls1x_nand_stop(info);
        }
        *(info->data_buff) = (nand_readl(info, NAND_IDH) >> 8) & 0xff;
        *(info->data_buff) |= 0x80;
        break;

    case NAND_CMD_READID:
        info->buf_count = 0x5;
        info->buf_start = 0;
        nand_writel(info, NAND_CMD, READ_ID);
        ls1x_nand_start(info);
        if (!ls1x_nand_done(info)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Wait time out!\r\n");
            ls1x_nand_stop(info);
            break;
        }
        *(info->data_buff) = nand_readl(info, NAND_IDH);
        *(info->data_buff + 1) = (nand_readl(info, NAND_IDL) >> 24) & 0xff;
        *(info->data_buff + 2) = (nand_readl(info, NAND_IDL) >> 16) & 0xff;
        *(info->data_buff + 3) = (nand_readl(info, NAND_IDL) >> 8) & 0xff;
        *(info->data_buff + 4) =  nand_readl(info, NAND_IDL) &  0xff;
        break;

    case NAND_CMD_ERASE2:
        info->buf_count = 0x0;
        info->buf_start = 0x0;
        break;

    default :
        info->buf_count = 0x0;
        info->buf_start = 0x0;
        _DebugHandle(__ERRORMESSAGE_LEVEL, "NON-Supported command!\r\n");
        break;
    }

    nand_writel(info, NAND_CMD, 0x00);
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_init_hw
** 功能描述: NAND HW Init
** 输　入  : ls1x_nand_info
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID ls1x_nand_init_hw(struct ls1x_nand_info *info)
{
    nand_writel(info, NAND_CMD, 0x00);
    nand_writel(info, NAND_ADDR_L, 0x00);
    nand_writel(info, NAND_ADDR_H, 0x00);
    nand_writel(info, NAND_TIMING, (HOLD_CYCLE << 8) | WAIT_CYCLE);
#if defined(LS1ASOC) || defined(LS1BSOC)
    nand_writel(info, NAND_PARAM, 0x00000100);                          /* 设置外部颗粒大小，1A 2Gb     */
#elif defined(LS1CSOC)
    nand_writel(info, NAND_PARAM, 0x08005000);
#endif
    nand_writel(info, NAND_OPNUM, 0x00);
    /* 重映射rdy1/2/3信号到rdy0 rdy用于判断是否忙 */
    nand_writel(info, NAND_CS_RDY, 0x88442211);
}
/*********************************************************************************************************
** 函数名称: ls1x_nand_scan
** 功能描述: NAND 扫描
** 输　入  : mtd_info
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static int ls1x_nand_scan(struct mtd_info *mtd)
{
    struct ls1x_nand_info *info = mtd->priv;
    struct nand_chip *chip = (struct nand_chip *)info;
    uint64_t chipsize;
    int exit_nand_size;

#ifndef SYLIXOS
    if (nand_scan_ident(mtd, 1))
        return -ENODEV;
#else
    if (nand_scan_ident(mtd, 1, NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Nand: NO Found!\r\n");
        return -ENODEV;
    }
#endif

    chipsize = (chip->chipsize << 3) >> 20;                             /* Mbit                         */

#ifdef SYLIXOS
    printk("Nand: %d MiB\n", (INT)(chipsize / 8));
#endif

    switch (mtd->writesize) {
    case 2048:
        switch (chipsize) {
        case 1024:
#if defined(LS1ASOC)
            exit_nand_size = 0x1;
#else
            exit_nand_size = 0x0;
#endif
            break;

        case 2048:
            exit_nand_size = 0x1;
            break;

        case 4096:
            exit_nand_size = 0x2;
            break;

        case 8192:
            exit_nand_size = 0x3;
            break;

        default:
            exit_nand_size = 0x3;
            break;
        }
        break;

    case 4096:
        exit_nand_size = 0x4;
        break;

    case 8192:
        switch (chipsize) {
        case 32768:
            exit_nand_size = 0x5;
            break;

        case 65536:
            exit_nand_size = 0x6;
            break;

        case 131072:
            exit_nand_size = 0x7;
            break;

        default:
            exit_nand_size = 0x8;
            break;
        }
        break;

    case 512:
        switch (chipsize) {
        case 64:
            exit_nand_size = 0x9;
            break;
        case 128:
            exit_nand_size = 0xa;
            break;
        case 256:
            exit_nand_size = 0xb;
            break;
        case 512:
            exit_nand_size = 0xc;
            break;
        default:
            exit_nand_size = 0xd;
            break;
        }
        break;

    default:
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Exit nand size error!\r\n");
        return -ENODEV;
    }
    nand_writel(info, NAND_PARAM,
            (nand_readl(info, NAND_PARAM) & 0xfffff0ff) | (exit_nand_size << 8));
    chip->cmdfunc(mtd, NAND_CMD_RESET, 0, 0);

    return nand_scan_tail(mtd);
}
#define ALIGN(x, a)             __ALIGN_MASK((x), (typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x) + (mask)) & ~(mask))

/*********************************************************************************************************
** 函数名称: ls1x_nand_init_buff
** 功能描述: NAND Init Buffer
** 输　入  : ls1x_nand_info
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT ls1x_nand_init_buff(struct ls1x_nand_info *info)
{
    /* DMA描述符地址 */
    info->dma_desc_size = ALIGN(DMA_DESC_NUM, PAGE_SIZE);               /* 申请内存大小，页对齐         */
#ifndef SYLIXOS
    info->dma_desc = ((UINT)malloc(info->dma_desc_size) & 0x0fffffff) | 0xa0000000;
#else
    info->dma_desc = ((UINT)API_VmmDmaAlloc(info->dma_desc_size) & 0x0fffffff) | 0xa0000000;
#endif
    info->dma_desc = (UINT)ALIGN((UINT)info->dma_desc, 32); /* 地址32字节对齐 */
    if(info->dma_desc == 0)
        return -1;
    info->dma_desc_phys = (UINT)(info->dma_desc) & 0x1fffffff;

    /* NAND的DMA数据缓存 */
    info->data_buff_size = ALIGN(MAX_BUFF_SIZE, PAGE_SIZE);             /* 申请内存大小，页对齐         */
#ifndef SYLIXOS
    info->data_buff = (UCHAR *)(((UINT)malloc(info->data_buff_size) & 0x0fffffff) | 0xa0000000);
#else
    info->data_buff = (UCHAR *)(((UINT)API_VmmDmaAlloc(info->data_buff_size) & 0x0fffffff) | 0xa0000000);
#endif
    info->data_buff = (UCHAR *)ALIGN((UINT)info->data_buff, 32);    /* 地址32字节对齐   */
    if(info->data_buff == NULL)
        return -1;
    info->data_buff_phys = (UINT)(info->data_buff) & 0x1fffffff;

    order_addr_in = ORDER_ADDR_IN;

#ifndef SYLIXOS
    printk("data_buff_addr:0x%08x, dma_addr:0x%08x\n", info->data_buff, info->dma_desc);
#endif

    return 0;
}
/*********************************************************************************************************
** 函数名称: nand_init
** 功能描述: NAND Device Init
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT ls1x_nand_init (VOID)
{
    struct ls1x_nand_info   *info;
    struct nand_chip        *chip;

    /* Allocate memory for MTD device structure and private data */
    ls1x_mtd = malloc(sizeof(struct mtd_info) + sizeof(struct ls1x_nand_info));
    if (!ls1x_mtd) {
        printf("Unable to allocate NAND MTD device structure.\n");
        return -ENOMEM;
    }

    info = (struct ls1x_nand_info *)(&ls1x_mtd[1]);
    chip = (struct nand_chip *)(&ls1x_mtd[1]);
    memset(ls1x_mtd, 0, sizeof(struct mtd_info));
    memset(info, 0, sizeof(struct ls1x_nand_info));

    ls1x_mtd->priv = info;

    chip->options       = NAND_CACHEPRG;
    chip->ecc.mode      = NAND_ECC_SOFT;
    chip->waitfunc      = ls1x_nand_waitfunc;
    chip->select_chip   = ls1x_nand_select_chip;
    chip->dev_ready     = ls1x_nand_dev_ready;
    chip->cmdfunc       = ls1x_nand_cmdfunc;
    chip->read_word     = ls1x_nand_read_word;
    chip->read_byte     = ls1x_nand_read_byte;
    chip->read_buf      = ls1x_nand_read_buf;
    chip->write_buf     = ls1x_nand_write_buf;
    chip->verify_buf    = ls1x_nand_verify_buf;

    info->mmio_base     = LS1X_NAND_BASE;

    if (ls1x_nand_init_buff(info)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "NANDFlash driver have some error!\r\n");
        return -ENXIO;
    }

    nand_gpio_init();
    ls1x_nand_init_hw(info);

    /* Scan to find existence of the device */
    if (ls1x_nand_scan(ls1x_mtd)) {
        free(ls1x_mtd);
        return -ENXIO;
    }

    /* Register the partitions */
    ls1x_mtd->name = "ls1x-nand";

#ifdef FAST_STARTUP
    if (output_mode == 0) {
        add_mtd_device(ls1x_mtd, 0, 0x400000, "kernel");
        return 0;
    }
#endif

    add_mtd_device(ls1x_mtd);

    return 0;
}
/*********************************************************************************************************
** 函数名称: nand_init
** 功能描述: NAND Device Init
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID nand_init (VOID)
{
    ls1x_nand_init();
}

/******************************************************************************/
/* 用于nand flash 环境变量读写的函数 */
#ifndef SYLIXOS
INT nand_probe_boot(VOID)
{
    return 0;
}

INT nand_erase_boot(VOID *base, INT size)
{
    INT page_addr;
    INT page_per_block = ls1x_mtd->erasesize / ls1x_mtd->writesize;
    INT erase_num = 0;

    page_addr = (UINT)base / ls1x_mtd->writesize;

    erase_num = size / ls1x_mtd->erasesize;
    if (size % ls1x_mtd->erasesize != 0)
        erase_num++;

    while (erase_num > 0) {
        ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_ERASE1, 0, page_addr);
        erase_num--;
        page_addr += page_per_block;
    }

    return 0;
}

INT nand_write_boot(VOID *base, VOID *buffer, INT size)
{
    struct ls1x_nand_info *info = ls1x_mtd->priv;
    UCHAR *data_buf = (UCHAR *)buffer;
    UINT page_addr = (UINT)base / ls1x_mtd->writesize;
    UINT buffer_size = size;

    while (buffer_size > ls1x_mtd->writesize) {
        ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_SEQIN, 0x00, page_addr);
        memcpy(info->data_buff, data_buf, ls1x_mtd->writesize);
        ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_PAGEPROG, 0, -1);

        buffer_size -= ls1x_mtd->writesize;
        data_buf += ls1x_mtd->writesize;
        page_addr += 1;
    }
    if (buffer_size) {
        ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_SEQIN, 0x00, page_addr);
        memcpy(info->data_buff, data_buf, buffer_size);
        ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_PAGEPROG, 0, -1);
    }
    return 0;
}

INT nand_read_boot(VOID *base, VOID *buffer, INT size)
{
    struct ls1x_nand_info *info = ls1x_mtd->priv;
    UCHAR *data_buf = (UCHAR *)buffer;
    UINT page_addr = (UINT)base / ls1x_mtd->writesize;
    UINT buffer_size = size;

    while (buffer_size > ls1x_mtd->writesize) {
        ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_READ0, 0x00, page_addr);
        memcpy(data_buf, info->data_buff, ls1x_mtd->writesize);

        buffer_size -= ls1x_mtd->writesize;
        data_buf += ls1x_mtd->writesize;
        page_addr += 1;
    }
    if (buffer_size) {
        ls1x_nand_cmdfunc(ls1x_mtd, NAND_CMD_READ0, 0x00, page_addr);
        memcpy(data_buf, info->data_buff, buffer_size);
    }

    return 0;
}

INT nand_verify_boot(VOID *base, VOID *buffer, INT size)
{
    UCHAR *r_buf = NULL;
    INT i = 0;

    r_buf = malloc(size);
    if (!r_buf) {
        printf("Unable to allocate NAND MTD device structure.\n");
        return -ENOMEM;
    }

    nand_read_boot(base, r_buf, size);

    while (size--) {
        if (*((UCHAR *)buffer + i) != r_buf[i]) {
            printf("verify error..., i= %d !\n\n", i-1);
            free(r_buf);
            return -1;
        }
        i++;
    }

    free(r_buf);

    return 0;
}

/* 只检测nand flash 0地址第0页 */
INT ls1x_nand_test(VOID)
{
    UCHAR *buffer = NULL;
    INT *nand_addr = 0;
    INT i, ret = 0;

    if (nand_scan_ident(ls1x_mtd, 1)) {
        return -ENODEV;
    }

    buffer = malloc(ls1x_mtd->writesize);
    if (!buffer) {
        printf("Unable to allocate NAND MTD device structure.\n");
        return -ENOMEM;
    }

    for (i=0; i<ls1x_mtd->writesize; i++) {
        *(buffer + i) = i;
    }

    nand_erase_boot(nand_addr, ls1x_mtd->writesize);
    nand_write_boot(nand_addr, buffer, ls1x_mtd->writesize);
    ret = nand_verify_boot(nand_addr, buffer, ls1x_mtd->writesize);

    free(buffer);

    return ret;
}
#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
