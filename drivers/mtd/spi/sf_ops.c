/*
 * SPI flash operations
 *
 * Copyright (C) 2008 Atmel Corporation
 * Copyright (C) 2010 Reinhard Meyer, EMK Elektronik
 * Copyright (C) 2013 Jagannadha Sutradharudu Teki, Xilinx Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <spi.h>
#include <spi_flash.h>
#include <watchdog.h>

#include "sf_internal.h"

/* Added by acaldwe */
#include <asm/armv7.h>
#define _ADI_MSK_3( mask, smask, type ) ((type)(smask))

#define REG_DMA8_DSCPTR_NXT                  0x31028000            /*  DMA8 Pointer to Next Initial Descriptor Register */
#define REG_DMA8_ADDRSTART                   0x31028004            /*  DMA8 Start Address of Current Buffer Register */
#define REG_DMA8_CFG                         0x31028008            /*  DMA8 Configuration Register */
#define REG_DMA8_XCNT                        0x3102800C            /*  DMA8 Inner Loop Count Start Value Register */
#define REG_DMA8_XMOD                        0x31028010            /*  DMA8 Inner Loop Address Increment Register */
#define REG_DMA8_YCNT                        0x31028014            /*  DMA8 Outer Loop Count Start Value (2D only) Register */
#define REG_DMA8_YMOD                        0x31028018            /*  DMA8 Outer Loop Address Increment (2D only) Register */
#define REG_DMA8_DSCPTR_CUR                  0x31028024            /*  DMA8 Current Descriptor Pointer Register */
#define REG_DMA8_DSCPTR_PRV                  0x31028028            /*  DMA8 Previous Initial Descriptor Pointer Register */
#define REG_DMA8_ADDR_CUR                    0x3102802C            /*  DMA8 Current Address Register */
#define REG_DMA8_STAT                        0x31028030            /*  DMA8 Status Register */
#define REG_DMA8_XCNT_CUR                    0x31028034            /*  DMA8 Current Count (1D) or Intra-row XCNT (2D) Register */
#define REG_DMA8_YCNT_CUR                    0x31028038            /*  DMA8 Current Row Count (2D only) Register */
#define REG_DMA8_BWLCNT                      0x31028040            /*  DMA8 Bandwidth Limit Count Register */
#define REG_DMA8_BWLCNT_CUR                  0x31028044            /*  DMA8 Bandwidth Limit Count Current Register */
#define REG_DMA8_BWMCNT                      0x31028048            /*  DMA8 Bandwidth Monitor Count Register */
#define REG_DMA8_BWMCNT_CUR                  0x3102804C            /*  DMA8 Bandwidth Monitor Count Current Register */

#define REG_DMA9_DSCPTR_NXT                  0x31028080            /*  DMA9 Pointer to Next Initial Descriptor Register */
#define REG_DMA9_ADDRSTART                   0x31028084            /*  DMA9 Start Address of Current Buffer Register */
#define REG_DMA9_CFG                         0x31028088            /*  DMA9 Configuration Register */
#define REG_DMA9_XCNT                        0x3102808C            /*  DMA9 Inner Loop Count Start Value Register */
#define REG_DMA9_XMOD                        0x31028090            /*  DMA9 Inner Loop Address Increment Register */
#define REG_DMA9_YCNT                        0x31028094            /*  DMA9 Outer Loop Count Start Value (2D only) Register */
#define REG_DMA9_YMOD                        0x31028098            /*  DMA9 Outer Loop Address Increment (2D only) Register */
#define REG_DMA9_DSCPTR_CUR                  0x310280A4            /*  DMA9 Current Descriptor Pointer Register */
#define REG_DMA9_DSCPTR_PRV                  0x310280A8            /*  DMA9 Previous Initial Descriptor Pointer Register */
#define REG_DMA9_ADDR_CUR                    0x310280AC            /*  DMA9 Current Address Register */
#define REG_DMA9_STAT                        0x310280B0            /*  DMA9 Status Register */
#define REG_DMA9_XCNT_CUR                    0x310280B4            /*  DMA9 Current Count (1D) or Intra-row XCNT (2D) Register */
#define REG_DMA9_YCNT_CUR                    0x310280B8            /*  DMA9 Current Row Count (2D only) Register */
#define REG_DMA9_BWLCNT                      0x310280C0            /*  DMA9 Bandwidth Limit Count Register */
#define REG_DMA9_BWLCNT_CUR                  0x310280C4            /*  DMA9 Bandwidth Limit Count Current Register */
#define REG_DMA9_BWMCNT                      0x310280C8            /*  DMA9 Bandwidth Monitor Count Register */
#define REG_DMA9_BWMCNT_CUR                  0x310280CC            /*  DMA9 Bandwidth Monitor Count Current Register */




#define BITP_DMA_CFG_PDRF                    28            /*  Peripheral Data Request Forward */
#define BITP_DMA_CFG_TWOD                    26            /*  Two Dimension Addressing Enable */
#define BITP_DMA_CFG_DESCIDCPY               25            /*  Descriptor ID Copy Control */
#define BITP_DMA_CFG_TOVEN                   24            /*  Trigger Overrun Error Enable */
#define BITP_DMA_CFG_TRIG                    22            /*  Generate Outgoing Trigger */
#define BITP_DMA_CFG_INT                     20            /*  Generate Interrupt Request */
#define BITP_DMA_CFG_NDSIZE                  16            /*  Next Descriptor Set Size */
#define BITP_DMA_CFG_TWAIT                   15            /*  Wait for Trigger */
#define BITP_DMA_CFG_FLOW                    12            /*  Next Operation */
#define BITP_DMA_CFG_MSIZE                    8            /*  Memory Transfer Word Size */
#define BITP_DMA_CFG_PSIZE                    4            /*  Peripheral Transfer Word Size */
#define BITP_DMA_CFG_CADDR                    3            /*  Use Current Address */
#define BITP_DMA_CFG_SYNC                     2            /*  Synchronize Work Unit Transitions */
#define BITP_DMA_CFG_WNR                      1            /*  Write/Read Channel Direction */
#define BITP_DMA_CFG_EN                       0            /*  DMA Channel Enable */
#define BITM_DMA_CFG_PDRF                    (_ADI_MSK_3(0x10000000,0x10000000UL, uint32_t  ))    /*  Peripheral Data Request Forward */
#define BITM_DMA_CFG_TWOD                    (_ADI_MSK_3(0x04000000,0x04000000UL, uint32_t  ))    /*  Two Dimension Addressing Enable */
#define BITM_DMA_CFG_DESCIDCPY               (_ADI_MSK_3(0x02000000,0x02000000UL, uint32_t  ))    /*  Descriptor ID Copy Control */
#define BITM_DMA_CFG_TOVEN                   (_ADI_MSK_3(0x01000000,0x01000000UL, uint32_t  ))    /*  Trigger Overrun Error Enable */
#define BITM_DMA_CFG_TRIG                    (_ADI_MSK_3(0x00C00000,0x00C00000UL, uint32_t  ))    /*  Generate Outgoing Trigger */
#define BITM_DMA_CFG_INT                     (_ADI_MSK_3(0x00300000,0x00300000UL, uint32_t  ))    /*  Generate Interrupt Request */
#define BITM_DMA_CFG_NDSIZE                  (_ADI_MSK_3(0x00070000,0x00070000UL, uint32_t  ))    /*  Next Descriptor Set Size */
#define BITM_DMA_CFG_TWAIT                   (_ADI_MSK_3(0x00008000,0x00008000UL, uint32_t  ))    /*  Wait for Trigger */
#define BITM_DMA_CFG_FLOW                    (_ADI_MSK_3(0x00007000,0x00007000UL, uint32_t  ))    /*  Next Operation */
#define BITM_DMA_CFG_MSIZE                   (_ADI_MSK_3(0x00000700,0x00000700UL, uint32_t  ))    /*  Memory Transfer Word Size */
#define BITM_DMA_CFG_PSIZE                   (_ADI_MSK_3(0x00000070,0x00000070UL, uint32_t  ))    /*  Peripheral Transfer Word Size */
#define BITM_DMA_CFG_CADDR                   (_ADI_MSK_3(0x00000008,0x00000008UL, uint32_t  ))    /*  Use Current Address */
#define BITM_DMA_CFG_SYNC                    (_ADI_MSK_3(0x00000004,0x00000004UL, uint32_t  ))    /*  Synchronize Work Unit Transitions */
#define BITM_DMA_CFG_WNR                     (_ADI_MSK_3(0x00000002,0x00000002UL, uint32_t  ))    /*  Write/Read Channel Direction */
#define BITM_DMA_CFG_EN                      (_ADI_MSK_3(0x00000001,0x00000001UL, uint32_t  ))    /*  DMA Channel Enable */

#define BITM_DMA_STAT_TWAIT                  (_ADI_MSK_3(0x00100000,0x00100000UL, uint32_t  ))    /*  Trigger Wait Status */
#define BITM_DMA_STAT_FIFOFILL               (_ADI_MSK_3(0x00070000,0x00070000UL, uint32_t  ))    /*  FIFO Fill Status */
#define BITM_DMA_STAT_MBWID                  (_ADI_MSK_3(0x0000C000,0x0000C000UL, uint32_t  ))    /*  Memory Bus Width */
#define BITM_DMA_STAT_PBWID                  (_ADI_MSK_3(0x00003000,0x00003000UL, uint32_t  ))    /*  Peripheral Bus Width */
#define BITM_DMA_STAT_RUN                    (_ADI_MSK_3(0x00000700,0x00000700UL, uint32_t  ))    /*  Run Status */
#define BITM_DMA_STAT_ERRC                   (_ADI_MSK_3(0x00000070,0x00000070UL, uint32_t  ))    /*  Error Cause */
#define BITM_DMA_STAT_PIRQ                   (_ADI_MSK_3(0x00000004,0x00000004UL, uint32_t  ))    /*  Peripheral Interrupt Request */
#define BITM_DMA_STAT_IRQERR                 (_ADI_MSK_3(0x00000002,0x00000002UL, uint32_t  ))    /*  Error Interrupt Request */
#define BITM_DMA_STAT_IRQDONE                (_ADI_MSK_3(0x00000001,0x00000001UL, uint32_t  ))    /*  Work Unit/Row Done Interrupt */

#define ENUM_DMA_CFG_PDAT_NOTFWD             (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  PDRF: Peripheral Data Request Not Forwarded */
#define ENUM_DMA_CFG_PDAT_FWD                (_ADI_MSK_3(0x10000000,0x10000000UL, uint32_t  ))    /*  PDRF: Peripheral Data Request Forwarded */
#define ENUM_DMA_CFG_ADDR1D                  (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  TWOD: One-Dimensional Addressing */
#define ENUM_DMA_CFG_ADDR2D                  (_ADI_MSK_3(0x04000000,0x04000000UL, uint32_t  ))    /*  TWOD: Two-Dimensional Addressing */
#define ENUM_DMA_CFG_NO_COPY                 (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  DESCIDCPY: Never Copy */
#define ENUM_DMA_CFG_COPY                    (_ADI_MSK_3(0x02000000,0x02000000UL, uint32_t  ))    /*  DESCIDCPY: Copy on Work Unit Complete */
#define ENUM_DMA_CFG_TOV_DIS                 (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  TOVEN: Ignore Trigger Overrun */
#define ENUM_DMA_CFG_TOV_EN                  (_ADI_MSK_3(0x01000000,0x01000000UL, uint32_t  ))    /*  TOVEN: Error on Trigger Overrun */
#define ENUM_DMA_CFG_NO_TRIG                 (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  TRIG: Never Assert Trigger */
#define ENUM_DMA_CFG_XCNT_TRIG               (_ADI_MSK_3(0x00400000,0x00400000UL, uint32_t  ))    /*  TRIG: Trigger When XCNTCUR Reaches 0 */
#define ENUM_DMA_CFG_YCNT_TRIG               (_ADI_MSK_3(0x00800000,0x00800000UL, uint32_t  ))    /*  TRIG: Trigger When YCNTCUR Reaches 0 */
#define ENUM_DMA_CFG_NO_INT                  (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  INT: Never Assert Interrupt */
#define ENUM_DMA_CFG_XCNT_INT                (_ADI_MSK_3(0x00100000,0x00100000UL, uint32_t  ))    /*  INT: Interrupt When X Count Expires */
#define ENUM_DMA_CFG_YCNT_INT                (_ADI_MSK_3(0x00200000,0x00200000UL, uint32_t  ))    /*  INT: Interrupt When Y Count Expires */
#define ENUM_DMA_CFG_PERIPH_INT              (_ADI_MSK_3(0x00300000,0x00300000UL, uint32_t  ))    /*  INT: Peripheral Interrupt request */
#define ENUM_DMA_CFG_FETCH01                 (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  NDSIZE: Fetch One Descriptor Element */
#define ENUM_DMA_CFG_FETCH02                 (_ADI_MSK_3(0x00010000,0x00010000UL, uint32_t  ))    /*  NDSIZE: Fetch Two Descriptor Elements */
#define ENUM_DMA_CFG_FETCH03                 (_ADI_MSK_3(0x00020000,0x00020000UL, uint32_t  ))    /*  NDSIZE: Fetch Three Descriptor Elements */
#define ENUM_DMA_CFG_FETCH04                 (_ADI_MSK_3(0x00030000,0x00030000UL, uint32_t  ))    /*  NDSIZE: Fetch Four Descriptor Elements */
#define ENUM_DMA_CFG_FETCH05                 (_ADI_MSK_3(0x00040000,0x00040000UL, uint32_t  ))    /*  NDSIZE: Fetch Five Descriptor Elements */
#define ENUM_DMA_CFG_FETCH06                 (_ADI_MSK_3(0x00050000,0x00050000UL, uint32_t  ))    /*  NDSIZE: Fetch Six Descriptor Elements */
#define ENUM_DMA_CFG_FETCH07                 (_ADI_MSK_3(0x00060000,0x00060000UL, uint32_t  ))    /*  NDSIZE: Fetch Seven Descriptor Elements */
#define ENUM_DMA_CFG_NO_TRGWAIT              (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  TWAIT: Begin Work Unit Automatically (No Wait) */
#define ENUM_DMA_CFG_TRGWAIT                 (_ADI_MSK_3(0x00008000,0x00008000UL, uint32_t  ))    /*  TWAIT: Wait for Trigger (Halt before Work Unit) */
#define ENUM_DMA_CFG_STOP                    (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  FLOW: STOP. */
#define ENUM_DMA_CFG_AUTO                    (_ADI_MSK_3(0x00001000,0x00001000UL, uint32_t  ))    /*  FLOW: AUTO. */
#define ENUM_DMA_CFG_DSCLIST                 (_ADI_MSK_3(0x00004000,0x00004000UL, uint32_t  ))    /*  FLOW: DSCL. */
#define ENUM_DMA_CFG_DSCARRAY                (_ADI_MSK_3(0x00005000,0x00005000UL, uint32_t  ))    /*  FLOW: DSCA. */
#define ENUM_DMA_CFG_DODLIST                 (_ADI_MSK_3(0x00006000,0x00006000UL, uint32_t  ))    /*  FLOW: Descriptor On-Demand List. */
#define ENUM_DMA_CFG_DODARRAY                (_ADI_MSK_3(0x00007000,0x00007000UL, uint32_t  ))    /*  FLOW: Descriptor On Demand Array. */
#define ENUM_DMA_CFG_MSIZE04                 (_ADI_MSK_3(0x00000200,0x00000200UL, uint32_t  ))    /*  MSIZE: 4 Bytes */
#define ENUM_DMA_CFG_MSIZE08                 (_ADI_MSK_3(0x00000300,0x00000300UL, uint32_t  ))    /*  MSIZE: 8 Bytes */
#define ENUM_DMA_CFG_MSIZE16                 (_ADI_MSK_3(0x00000400,0x00000400UL, uint32_t  ))    /*  MSIZE: 16 Bytes */
#define ENUM_DMA_CFG_MSIZE32                 (_ADI_MSK_3(0x00000500,0x00000500UL, uint32_t  ))    /*  MSIZE: 32 Bytes */
#define ENUM_DMA_CFG_PSIZE04                 (_ADI_MSK_3(0x00000020,0x00000020UL, uint32_t  ))    /*  PSIZE: 4 Bytes */
#define ENUM_DMA_CFG_PSIZE08                 (_ADI_MSK_3(0x00000030,0x00000030UL, uint32_t  ))    /*  PSIZE: 8 Bytes */
#define ENUM_DMA_CFG_LD_STARTADDR            (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  CADDR: Load Starting Address */
#define ENUM_DMA_CFG_LD_CURADDR              (_ADI_MSK_3(0x00000008,0x00000008UL, uint32_t  ))    /*  CADDR: Use Current Address */
#define ENUM_DMA_CFG_NO_SYNC                 (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  SYNC: No Synchronization */
#define ENUM_DMA_CFG_SYNC                    (_ADI_MSK_3(0x00000004,0x00000004UL, uint32_t  ))    /*  SYNC: Synchronize Channel */
#define ENUM_DMA_CFG_READ                    (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  WNR: Transmit (Read from memory) */
#define ENUM_DMA_CFG_WRITE                   (_ADI_MSK_3(0x00000002,0x00000002UL, uint32_t  ))    /*  WNR: Receive (Write to memory) */
#define ENUM_DMA_CFG_DIS                     (_ADI_MSK_3(0x00000000,0x00000000UL, uint32_t  ))    /*  EN: Disable */
#define ENUM_DMA_CFG_EN                      (_ADI_MSK_3(0x00000001,0x00000001UL, uint32_t  ))    /*  EN: Enable */

#define DMA_MIN(a,b) (((a)<(b))?(a):(b))
#define DMA_MAX(a,b) (((a)>(b))?(a):(b))

#define BK_DMA_MDMA_SRC_DEFAULT_CONFIG(PSize, MSize) (BITM_DMA_CFG_EN|(PSize << BITP_DMA_CFG_PSIZE)|(MSize << BITP_DMA_CFG_MSIZE))
#define BK_DMA_MDMA_DST_DEFAULT_CONFIG(PSize, MSize) (BITM_DMA_CFG_EN|BITM_DMA_CFG_WNR|ENUM_DMA_CFG_XCNT_INT|(PSize << BITP_DMA_CFG_PSIZE)|(MSize << BITP_DMA_CFG_MSIZE))

#define MAX_DMA_PSIZE 2



typedef struct dma_regs {
	volatile u32 DSCPTR_NXT;
	volatile u32 ADDRSTART;
	volatile u32 CFG;
	volatile u32 XCNT;
	volatile u32 XMOD;
	volatile u32 YCNT;
	volatile u32 YMOD;
        volatile u32 reserved0;
        volatile u32 reserved1;
	volatile u32 DSCPTR_CUR;
	volatile u32 DSCPTR_PRV;
	volatile u32 ADDR_CUR;
	volatile u32 STAT;
	volatile u32 XCNT_CUR;
	volatile u32 YCNT_CUR;
        volatile u32 reserved3;
	volatile u32 BWLCNT;
	volatile u32 BWLCNT_CUR;
	volatile u32 BWMCNT;
	volatile u32 BWMCNT_CUR;
}dma_regs;

static u8 dma_get_msize(u32 nByteCount, u32 nAddress)
{
  /* Calculate MSIZE, PSIZE, XCNT and XMOD */
  u8  nMsize = 0;
  u32 nValue = nByteCount | nAddress;
  u32 nMask  = 0x1;


  for (nMsize = 0; nMsize < 5; nMsize++, nMask <<= 1) {
    if ((nValue & nMask) == nMask) {
      break;
    }
  }

  return nMsize;
}

static u32 memcopy_dma(void *data, void * flash_source, size_t len)
{
  struct dma_regs * mdma_src= (dma_regs *)REG_DMA8_DSCPTR_NXT;
  struct dma_regs * mdma_dest= (dma_regs *)REG_DMA9_DSCPTR_NXT;

  u32 result = 0x00000001; /* Default success */
  u32 ByteCount = (u32) len;

  u8 nSrcMsize ;
  u8 nDstMsize ;
  u8 nSrcPsize ;
  u8 nDstPsize ;
  u8 nPSize ;
  u32 SrcConfig ;
  u32 DstConfig ;

  /* guard against zero byte count */
  if (len == 0) {
    return 0x00000002;
  }

  /* Clear DMA status */
  mdma_src->STAT = (BITM_DMA_STAT_IRQDONE | BITM_DMA_STAT_IRQERR | BITM_DMA_STAT_PIRQ);
  mdma_dest->STAT = (BITM_DMA_STAT_IRQDONE | BITM_DMA_STAT_IRQERR | BITM_DMA_STAT_PIRQ);

  /* Calculate MSIZE, PSIZE, XCNT and XMOD */
  nSrcMsize = dma_get_msize(ByteCount, (u32) flash_source);
  nDstMsize = dma_get_msize(ByteCount, (u32) data);
  nSrcPsize = DMA_MIN(nSrcMsize, MAX_DMA_PSIZE);
  nDstPsize = DMA_MIN(nDstMsize, MAX_DMA_PSIZE);
  nPSize = DMA_MIN(nSrcPsize, nDstPsize);

  SrcConfig = BK_DMA_MDMA_SRC_DEFAULT_CONFIG(nPSize, nSrcMsize);
  DstConfig = BK_DMA_MDMA_DST_DEFAULT_CONFIG(nPSize, nDstMsize);

  /* Load the DMA descriptors */
  mdma_src->ADDRSTART = (u32)flash_source;
  mdma_src->XCNT = ByteCount >> nSrcMsize;
  mdma_src->XMOD = 1 << nSrcMsize;
  mdma_dest->ADDRSTART = (u32)data;
  mdma_dest->XCNT = ByteCount >> nDstMsize;
  mdma_dest->XMOD = 1 << nDstMsize;

  CP15ISB;
  CP15DSB;
  CP15DMB;

  mdma_dest->CFG = DstConfig;
  mdma_src->CFG = SrcConfig;

  CP15ISB;
  CP15DSB;
  CP15DMB;

  /* Check for any configuration errors */
  if ((mdma_src->STAT & BITM_DMA_STAT_IRQERR) == BITM_DMA_STAT_IRQERR) {
    result = 0x00000003;
  }
  if ((mdma_dest->STAT & BITM_DMA_STAT_IRQERR) == BITM_DMA_STAT_IRQERR) {
    result = 0x00000004;
  }

      /* Wait for DMA to complete while checking for a DMA error */
      do {
        if ((mdma_src->STAT & BITM_DMA_STAT_IRQERR) == BITM_DMA_STAT_IRQERR) {
          mdma_src->CFG &= ~1;
          mdma_dest->CFG &= ~1;
          return 0x00000005;
        }
        if ((mdma_dest->STAT & BITM_DMA_STAT_IRQERR) == BITM_DMA_STAT_IRQERR) {
          mdma_src->CFG &= ~1;
          mdma_dest->CFG &= ~1;
          return 0x00000006;
        }
      }while ((mdma_dest->STAT & BITM_DMA_STAT_IRQDONE) == 0);

      mdma_src->CFG &= ~1;
      mdma_dest->CFG &= ~1;
      return result;
}

/* End of edits by acaldwe */







static void spi_flash_addr(u32 addr, u8 *cmd)
{
	/* cmd[0] is actual command */
	cmd[1] = addr >> 16;
	cmd[2] = addr >> 8;
	cmd[3] = addr >> 0;
}

int spi_flash_cmd_read_status(struct spi_flash *flash, u8 *rs)
{
	int ret;
	u8 cmd;

	cmd = CMD_READ_STATUS;
	ret = spi_flash_read_common(flash, &cmd, 1, rs, 1);
	if (ret < 0) {
		debug("SF: fail to read status register\n");
		return ret;
	}

	return 0;
}

int spi_flash_cmd_write_status(struct spi_flash *flash, u8 ws)
{
	u8 cmd;
	int ret;

	cmd = CMD_WRITE_STATUS;
	ret = spi_flash_write_common(flash, &cmd, 1, &ws, 1);
	if (ret < 0) {
		debug("SF: fail to write status register\n");
		return ret;
	}

	return 0;
}

#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
int spi_flash_cmd_read_config(struct spi_flash *flash, u8 *rc)
{
	int ret;
	u8 cmd;

	cmd = CMD_READ_CONFIG;
	ret = spi_flash_read_common(flash, &cmd, 1, rc, 1);
	if (ret < 0) {
		debug("SF: fail to read config register\n");
		return ret;
	}

	return 0;
}

int spi_flash_cmd_write_config(struct spi_flash *flash, u8 wc)
{
	u8 data[2];
	u8 cmd;
	int ret;

	ret = spi_flash_cmd_read_status(flash, &data[0]);
	if (ret < 0)
		return ret;

	cmd = CMD_WRITE_STATUS;
	data[1] = wc;
	ret = spi_flash_write_common(flash, &cmd, 1, &data, 2);
	if (ret) {
		debug("SF: fail to write config register\n");
		return ret;
	}

	return 0;
}
#endif

#ifdef CONFIG_SPI_FLASH_BAR
static int spi_flash_cmd_bankaddr_write(struct spi_flash *flash, u8 bank_sel)
{
	u8 cmd;
	int ret;

	if (flash->bank_curr == bank_sel) {
		debug("SF: not require to enable bank%d\n", bank_sel);
		return 0;
	}

	cmd = flash->bank_write_cmd;
	ret = spi_flash_write_common(flash, &cmd, 1, &bank_sel, 1);
	if (ret < 0) {
		debug("SF: fail to write bank register\n");
		return ret;
	}
	flash->bank_curr = bank_sel;

	return 0;
}

static int spi_flash_bank(struct spi_flash *flash, u32 offset)
{
	u8 bank_sel;
	int ret;

	bank_sel = offset / (SPI_FLASH_16MB_BOUN << flash->shift);

	ret = spi_flash_cmd_bankaddr_write(flash, bank_sel);
	if (ret) {
		debug("SF: fail to set bank%d\n", bank_sel);
		return ret;
	}

	return bank_sel;
}
#endif

#ifdef CONFIG_SF_DUAL_FLASH
static void spi_flash_dual_flash(struct spi_flash *flash, u32 *addr)
{
	switch (flash->dual_flash) {
	case SF_DUAL_STACKED_FLASH:
		if (*addr >= (flash->size >> 1)) {
			*addr -= flash->size >> 1;
			flash->spi->flags |= SPI_XFER_U_PAGE;
		} else {
			flash->spi->flags &= ~SPI_XFER_U_PAGE;
		}
		break;
	case SF_DUAL_PARALLEL_FLASH:
		*addr >>= flash->shift;
		break;
	default:
		debug("SF: Unsupported dual_flash=%d\n", flash->dual_flash);
		break;
	}
}
#endif

int spi_flash_cmd_wait_ready(struct spi_flash *flash, unsigned long timeout)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timebase;
	unsigned long flags = SPI_XFER_BEGIN;
	int ret;
	u8 status;
	u8 check_status = 0x0;
	u8 poll_bit = STATUS_WIP;
	u8 cmd = flash->poll_cmd;

	if (cmd == CMD_FLAG_STATUS) {
		poll_bit = STATUS_PEC;
		check_status = poll_bit;
	}

#ifdef CONFIG_SF_DUAL_FLASH
	if (spi->flags & SPI_XFER_U_PAGE)
		flags |= SPI_XFER_U_PAGE;
#endif
	ret = spi_xfer(spi, 8, &cmd, NULL, flags);
	if (ret) {
		debug("SF: fail to read %s status register\n",
		      cmd == CMD_READ_STATUS ? "read" : "flag");
		return ret;
	}

	timebase = get_timer(0);
	do {
		WATCHDOG_RESET();

		ret = spi_xfer(spi, 8, NULL, &status, 0);
		if (ret)
			return -1;

		if ((status & poll_bit) == check_status)
			break;

	} while (get_timer(timebase) < timeout);

	spi_xfer(spi, 0, NULL, NULL, SPI_XFER_END);

	if ((status & poll_bit) == check_status)
		return 0;

	/* Timed out */
	debug("SF: time out!\n");
	return -1;
}

int spi_flash_write_common(struct spi_flash *flash, const u8 *cmd,
		size_t cmd_len, const void *buf, size_t buf_len)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timeout = SPI_FLASH_PROG_TIMEOUT;
	int ret;

	if (buf == NULL)
		timeout = SPI_FLASH_PAGE_ERASE_TIMEOUT;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: unable to claim SPI bus\n");
		return ret;
	}

	ret = spi_flash_cmd_write_enable(flash);
	if (ret < 0) {
		debug("SF: enabling write failed\n");
		return ret;
	}

	ret = spi_flash_cmd_write(spi, cmd, cmd_len, buf, buf_len);
	if (ret < 0) {
		debug("SF: write cmd failed\n");
		return ret;
	}

	ret = spi_flash_cmd_wait_ready(flash, timeout);
	if (ret < 0) {
		debug("SF: write %s timed out\n",
		      timeout == SPI_FLASH_PROG_TIMEOUT ?
			"program" : "page erase");
		return ret;
	}

	spi_release_bus(spi);

	return ret;
}

int spi_flash_cmd_erase_ops(struct spi_flash *flash, u32 offset, size_t len)
{
	u32 erase_size, erase_addr;
	u8 cmd[SPI_FLASH_CMD_LEN];
	int ret = -1;

	erase_size = flash->erase_size;
	if (offset % erase_size || len % erase_size) {
		debug("SF: Erase offset/length not multiple of erase size\n");
		return -1;
	}

	cmd[0] = flash->erase_cmd;
	while (len) {
		erase_addr = offset;

#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual_flash(flash, &erase_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		ret = spi_flash_bank(flash, erase_addr);
		if (ret < 0)
			return ret;
#endif
		spi_flash_addr(erase_addr, cmd);

		debug("SF: erase %2x %2x %2x %2x (%x)\n", cmd[0], cmd[1],
		      cmd[2], cmd[3], erase_addr);

		ret = spi_flash_write_common(flash, cmd, sizeof(cmd), NULL, 0);
		if (ret < 0) {
			debug("SF: erase failed\n");
			break;
		}

		offset += erase_size;
		len -= erase_size;
	}

	return ret;
}

int spi_flash_cmd_write_ops(struct spi_flash *flash, u32 offset,
		size_t len, const void *buf)
{
	unsigned long byte_addr, page_size;
	u32 write_addr;
	size_t chunk_len, actual;
	u8 cmd[SPI_FLASH_CMD_LEN];
	int ret = -1;

	page_size = flash->page_size;

	cmd[0] = flash->write_cmd;
	for (actual = 0; actual < len; actual += chunk_len) {
		write_addr = offset;

#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual_flash(flash, &write_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		ret = spi_flash_bank(flash, write_addr);
		if (ret < 0)
			return ret;
#endif
		byte_addr = offset % page_size;
		chunk_len = min(len - actual, (size_t)(page_size - byte_addr));

		if (flash->spi->max_write_size)
			chunk_len = min(chunk_len,
					(size_t)flash->spi->max_write_size);

		spi_flash_addr(write_addr, cmd);

		debug("SF: 0x%p => cmd = { 0x%02x 0x%02x%02x%02x } chunk_len = %zu\n",
		      buf + actual, cmd[0], cmd[1], cmd[2], cmd[3], chunk_len);

		ret = spi_flash_write_common(flash, cmd, sizeof(cmd),
					buf + actual, chunk_len);
		if (ret < 0) {
			debug("SF: write failed\n");
			break;
		}

		offset += chunk_len;
	}

	return ret;
}

int spi_flash_read_common(struct spi_flash *flash, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len)
{
	struct spi_slave *spi = flash->spi;
	int ret;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: unable to claim SPI bus\n");
		return ret;
	}

	ret = spi_flash_cmd_read(spi, cmd, cmd_len, data, data_len);
	if (ret < 0) {
		debug("SF: read cmd failed\n");
		return ret;
	}

	spi_release_bus(spi);

	return ret;
}

int spi_flash_cmd_read_ops(struct spi_flash *flash, u32 offset,
		size_t len, void *data)
{
	u8 *cmd, cmdsz;
	u32 remain_len, read_len, read_addr;
	int bank_sel = 0;
	int ret = -1;

	/* Handle memory-mapped SPI */
	if (flash->memory_map) {
		ret = spi_claim_bus(flash->spi);
		if (ret) {
			debug("SF: unable to claim SPI bus\n");
			return ret;
		}
		spi_xfer(flash->spi, 0, NULL, NULL, SPI_XFER_MMAP);
/* Edit by acaldwe */
		memcpy(data, flash->memory_map + offset, len);
		//u32 memcopy_result = 0xDEADBEEF;
		//memcopy_result = memcopy_dma(data,flash->memory_map + offset,len);
		//if(memcopy_result!= 0x00000001)
		//{
		//	printf("ADI Memcopy via DMA failure: 0x%.8x\n", memcopy_result);
		//}
/* End of edit by acaldwe */
		spi_xfer(flash->spi, 0, NULL, NULL, SPI_XFER_MMAP_END);
		spi_release_bus(flash->spi);
		return 0;
	}

	cmdsz = SPI_FLASH_CMD_LEN + flash->dummy_byte;
	cmd = calloc(1, cmdsz);
	if (!cmd) {
		debug("SF: Failed to allocate cmd\n");
		return -ENOMEM;
	}

	cmd[0] = flash->read_cmd;
	while (len) {
		read_addr = offset;

#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual_flash(flash, &read_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		bank_sel = spi_flash_bank(flash, read_addr);
		if (bank_sel < 0)
			return ret;
#endif
		remain_len = ((SPI_FLASH_16MB_BOUN << flash->shift) *
				(bank_sel + 1)) - offset;
		if (len < remain_len)
			read_len = len;
		else
			read_len = remain_len;

		spi_flash_addr(read_addr, cmd);

		ret = spi_flash_read_common(flash, cmd, cmdsz, data, read_len);
		if (ret < 0) {
			debug("SF: read failed\n");
			break;
		}

		offset += read_len;
		len -= read_len;
		data += read_len;
	}

	free(cmd);
	return ret;
}

#ifdef CONFIG_SPI_FLASH_SST
static int sst_byte_write(struct spi_flash *flash, u32 offset, const void *buf)
{
	int ret;
	u8 cmd[4] = {
		CMD_SST_BP,
		offset >> 16,
		offset >> 8,
		offset,
	};

	debug("BP[%02x]: 0x%p => cmd = { 0x%02x 0x%06x }\n",
	      spi_w8r8(flash->spi, CMD_READ_STATUS), buf, cmd[0], offset);

	ret = spi_flash_cmd_write_enable(flash);
	if (ret)
		return ret;

	ret = spi_flash_cmd_write(flash->spi, cmd, sizeof(cmd), buf, 1);
	if (ret)
		return ret;

	return spi_flash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
}

int sst_write_wp(struct spi_flash *flash, u32 offset, size_t len,
		const void *buf)
{
	size_t actual, cmd_len;
	int ret;
	u8 cmd[4];

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: Unable to claim SPI bus\n");
		return ret;
	}

	/* If the data is not word aligned, write out leading single byte */
	actual = offset % 2;
	if (actual) {
		ret = sst_byte_write(flash, offset, buf);
		if (ret)
			goto done;
	}
	offset += actual;

	ret = spi_flash_cmd_write_enable(flash);
	if (ret)
		goto done;

	cmd_len = 4;
	cmd[0] = CMD_SST_AAI_WP;
	cmd[1] = offset >> 16;
	cmd[2] = offset >> 8;
	cmd[3] = offset;

	for (; actual < len - 1; actual += 2) {
		debug("WP[%02x]: 0x%p => cmd = { 0x%02x 0x%06x }\n",
		      spi_w8r8(flash->spi, CMD_READ_STATUS), buf + actual,
		      cmd[0], offset);

		ret = spi_flash_cmd_write(flash->spi, cmd, cmd_len,
					buf + actual, 2);
		if (ret) {
			debug("SF: sst word program failed\n");
			break;
		}

		ret = spi_flash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
		if (ret)
			break;

		cmd_len = 1;
		offset += 2;
	}

	if (!ret)
		ret = spi_flash_cmd_write_disable(flash);

	/* If there is a single trailing byte, write it out */
	if (!ret && actual != len)
		ret = sst_byte_write(flash, offset, buf + actual);

 done:
	debug("SF: sst: program %s %zu bytes @ 0x%zx\n",
	      ret ? "failure" : "success", len, offset - actual);

	spi_release_bus(flash->spi);
	return ret;
}

int sst_write_bp(struct spi_flash *flash, u32 offset, size_t len,
		const void *buf)
{
	size_t actual;
	int ret;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: Unable to claim SPI bus\n");
		return ret;
	}

	for (actual = 0; actual < len; actual++) {
		ret = sst_byte_write(flash, offset, buf + actual);
		if (ret) {
			debug("SF: sst byte program failed\n");
			break;
		}
		offset++;
	}

	if (!ret)
		ret = spi_flash_cmd_write_disable(flash);

	debug("SF: sst: program %s %zu bytes @ 0x%zx\n",
	      ret ? "failure" : "success", len, offset - actual);

	spi_release_bus(flash->spi);
	return ret;
}
#endif
