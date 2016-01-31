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
** ��   ��   ��: ls1x_wdt.h
**
** ��   ��   ��: Ryan.Xin (�Ž���)
**
** �ļ���������: 2015 �� 12 �� 13 ��
**
** ��        ��: loongson1B NAND Flash ����ͷ�ļ�.
*********************************************************************************************************/
#ifndef LS1X_NAND_H_
#define LS1X_NAND_H_

#define DATA_BUFF               (0xa0000000)
#define DMA_DESC                (0xa0200000)

#define DMA_ACCESS_ADDR         (0x1fe78040)                            /*  DMA��NAND�����ĵ�ַ         */
#define ORDER_ADDR_IN           (0xbfd01160)                            /*  DMA���üĴ���               */
#define DMA_DESC_NUM            (64)                                    /*  DMA������ռ�õ��ֽ��� 7x4   */

/* DMA������ */
#define DMA_ORDERED             (0x00)
#define DMA_SADDR               (0x04)
#define DMA_DADDR               (0x08)
#define DMA_LENGTH              (0x0c)
#define DMA_STEP_LENGTH         (0x10)
#define DMA_STEP_TIMES          (0x14)
#define DMA_CMD                 (0x18)

/* registers and bit definitions */
#define LS1X_NAND_BASE          (0xbfe78000)
#define NAND_CMD                (0x00)                                  /*  Control register            */
#define NAND_ADDR_L             (0x04)                                  /*  ��д����������ʼ��ַ��32λ  */
#define NAND_ADDR_H             (0x08)                                  /*  ��д����������ʼ��ַ��8λ   */
#define NAND_TIMING             (0x0c)
#define NAND_IDL                (0x10)                                  /*  ID��32λ                    */
#define NAND_IDH                (0x14)                                  /*  ID��8λ                     */
#define NAND_STATUS             (0x14)                                  /*  Status Register             */
#define NAND_PARAM              (0x18)                                  /*  �ⲿ����������С            */
#define NAND_OPNUM              (0x1c)                                  /*  NAND��д�����ֽ���;�������� */
#define NAND_CS_RDY             (0x20)

/* NAND_TIMING�Ĵ������� */
#define HOLD_CYCLE              (0x02)
#define WAIT_CYCLE              (0x0c)

#define DMA_REQ                 (0x1 << 25)
#define ECC_DMA_REQ             (0x1 << 24)
#define WAIT_RSRD_DONE          (0x1 << 14)
#define INT_EN                  (0x1 << 13)
#define RS_WR                   (0x1 << 12)
#define RS_RD                   (0x1 << 11)
#define DONE                    (0x1 << 10)
#define SPARE                   (0x1 << 9)
#define MAIN                    (0x1 << 8)
#define READ_STATUS             (0x1 << 7)
#define RESET                   (0x1 << 6)
#define READ_ID                 (0x1 << 5)
#define BLOCKS_ERASE            (0x1 << 4)
#define ERASE                   (0x1 << 3)
#define WRITE                   (0x1 << 2)
#define READ                    (0x1 << 1)
#define COM_VALID               (0x1 << 0)

/* Macros for registers read/write */
#define nand_writel(info, off, val) \
    __raw_writel((val), (info)->mmio_base + (off))

#define nand_readl(info, off)       \
    __raw_readl((info)->mmio_base + (off))

/* 1MByte ������ʱ���䲻�ɹ� */
#define MAX_BUFF_SIZE           (10240)
#define PAGE_SHIFT              (12)                                    /* ҳ�ڵ�ַ(�е�ַ)A0-A11       */

#if defined(LS1ASOC) || defined(LS1CSOC)
    #define MAIN_ADDRH(x)       (x)
    #define MAIN_ADDRL(x)       ((x) << PAGE_SHIFT)
    #define MAIN_SPARE_ADDRH(x) (x)
    #define MAIN_SPARE_ADDRL(x) ((x) << PAGE_SHIFT)
#elif defined(LS1BSOC)
    #define MAIN_ADDRH(x)       ((x) >> (32 - (PAGE_SHIFT - 1)))
    #define MAIN_ADDRL(x)       ((x) << (PAGE_SHIFT - 1))               /* ������spare��ʱA11��Ч       */
    #define MAIN_SPARE_ADDRH(x) ((x) >> (32 - PAGE_SHIFT))
    #define MAIN_SPARE_ADDRL(x) ((x) << PAGE_SHIFT)
#endif

#endif                                                                  /*  LS1X_NAND_H_                */
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
