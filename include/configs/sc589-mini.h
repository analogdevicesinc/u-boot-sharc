/*
 * U-boot - Configuration file for sc589-mini board
 */

#ifndef __CONFIG_SC589_MINI_H
#define __CONFIG_SC589_MINI_H

#include <asm/arch/config.h>

/*
 * Processor Settings
 */
#define CONFIG_CPU		"ADSP-SC589-0.1"
#ifdef CONFIG_SC58X_CHAIN_BOOT
# define CONFIG_LOADADDR	0xC4000000
# define CONFIG_RSA		/* RSA for FIT authen. */
#else
# define CONFIG_LOADADDR	0xC2000000
#endif
#define CONFIG_DTBLOADADDR	"0xC4000000"
#define CONFIG_MACH_TYPE	MACH_TYPE_SC589_MINI
#define CONFIG_SYS_ARM_CACHE_WRITETHROUGH

/*
 * Clock Settings
 *	CCLK = (CLKIN * VCO_MULT) / CCLK_DIV
 *	SCLK = (CLKIN * VCO_MULT) / SYSCLK_DIV
 *	SCLK0 = SCLK / SCLK0_DIV
 *	SCLK1 = SCLK / SCLK1_DIV
 */
/* CONFIG_CLKIN_HZ is any value in Hz					*/
#define CONFIG_CLKIN_HZ			(25000000)
/* CLKIN_HALF controls the DF bit in PLL_CTL      0 = CLKIN		*/
/*                                                1 = CLKIN / 2		*/
#define CONFIG_CLKIN_HALF		(0)

#define CGU_ISSUE
/* VCO_MULT controls the MSEL (multiplier) bits in PLL_CTL		*/
/* Values can range from 0-127 (where 0 means 128)			*/
#ifdef CONFIG_CLOCK_SPEED_500MHZ
/* CONFIG_CCLK_DIV should be 1 in this case */
#define CONFIG_VCO_MULT			(20)
#define CONFIG_CGU1_VCO_MULT	(18)
#define CONFIG_CGU1_CCLK_DIV	(4)
#define CONFIG_CGU1_SCLK_DIV	(7)
#define CONFIG_CGU1_SCLK0_DIV	(2)
#define CONFIG_CGU1_SCLK1_DIV	(2)
#define CONFIG_CGU1_DCLK_DIV	(1)
#define CONFIG_CGU1_OCLK_DIV	(11)
#else
#define CONFIG_VCO_MULT			(18)
#endif
/* CCLK_DIV controls the core clock divider				*/
/* Values can range from 0-31 (where 0 means 32)			*/
#define CONFIG_CCLK_DIV			(1)
/* SCLK_DIV controls the system clock divider				*/
/* Values can range from 0-31 (where 0 means 32)			*/
#define CONFIG_SCLK_DIV			(2)
/* Values can range from 0-7 (where 0 means 8)				*/
#define CONFIG_SCLK0_DIV		(2)
#define CONFIG_SCLK1_DIV		(2)
/* DCLK_DIV controls the DDR clock divider				*/
/* Values can range from 0-31 (where 0 means 32)			*/
#ifdef CONFIG_CLOCK_SPEED_500MHZ
#define CONFIG_DCLK_DIV			(2)
#else
#define CONFIG_DCLK_DIV			(1)
#endif
/* OCLK_DIV controls the output clock divider				*/
/* Values can range from 0-127 (where 0 means 128)			*/
#ifdef CONFIG_CLOCK_SPEED_500MHZ
#define CONFIG_OCLK_DIV			(4)
#else
#define CONFIG_OCLK_DIV			(3)
#endif
#define CONFIG_VCO_HZ (CONFIG_CLKIN_HZ * CONFIG_VCO_MULT)
#define CONFIG_CCLK_HZ (CONFIG_VCO_HZ / CONFIG_CCLK_DIV)
#define CONFIG_SCLK_HZ (CONFIG_VCO_HZ / CONFIG_SCLK_DIV)

#define CONFIG_SYS_TIMERGROUP	TIMER_GROUP
#define CONFIG_SYS_TIMERBASE	TIMER0_CONFIG

/*
 * Memory Settings
 */
#define MEM_MT41K128M16JT
#define MEM_DMC0
#define MEM_DMC1

#define	CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_SDRAM_BASE	0xC2000000
#define CONFIG_SYS_SDRAM_SIZE	0xe000000
#define CONFIG_SYS_TEXT_BASE	0xC2200000
#define CONFIG_SYS_LOAD_ADDR	0x0
#define CONFIG_SYS_INIT_SP_ADDR (CONFIG_SYS_SDRAM_BASE + 0x3f000)

#define CONFIG_SMC_GCTL_VAL	0x00000010
#define CONFIG_SMC_B0CTL_VAL	0x01007011
#define CONFIG_SMC_B0TIM_VAL	0x08170977
#define CONFIG_SMC_B0ETIM_VAL	0x00092231
#ifndef CONFIG_SPL_BUILD
#define CONFIG_SYS_MONITOR_LEN	(0)
#else
#define CONFIG_SYS_MONITOR_LEN  (300 * 1024)
#endif
#define CONFIG_SYS_MALLOC_LEN	(1024 * 1024)

/* Reserve 4MB in DRAM for Tlb, Text, Malloc pool, Global data, Stack, etc. */
#define CONFIG_SYS_MEMTEST_RESERVE_SIZE	(4 * 1024 * 1024)
#define CONFIG_SYS_MEMTEST_START		CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END			(CONFIG_SYS_SDRAM_BASE + \
				CONFIG_SYS_SDRAM_SIZE - \
				CONFIG_SYS_MEMTEST_RESERVE_SIZE)

/*
 * Network Settings
 */
#define ADI_CMDS_NETWORK
#define CONFIG_NETCONSOLE
#define CONFIG_NET_MULTI
#define CONFIG_DTBNAME		"sc589-mini.dtb"
#define CONFIG_HOSTNAME		"sc58x"
#define CONFIG_DESIGNWARE_ETH
#define CONFIG_DW_AUTONEG
#define CONFIG_DW_ALTDESCRIPTOR
#define CONFIG_DW_AXI_BURST_LEN 16
#define CONFIG_MII
#define CONFIG_PHYLIB
#define CONFIG_PHY_TI
#define CONFIG_ETHADDR	02:80:ad:20:31:e8

/*
 * I2C Settings
 */
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_ADI
#define CONFIG_SYS_MAX_I2C_BUS 3


/*
 * SPI Settings
 */
#define CONFIG_ADI_SPI3
#define CONFIG_SC58X_SPI
#define CONFIG_CMD_SPI
#define CONFIG_ENV_SPI_MAX_HZ	50000000
#define CONFIG_SF_DEFAULT_SPEED	50000000
#define CONFIG_SPI_FLASH
/*#define CONFIG_SPI_FLASH_STMICRO*/
#define CONFIG_SPI_FLASH_ISSI
#define CONFIG_SPI_FLASH_BAR
#define CONFIG_SPI_FLASH_MMAP
#define CONFIG_SF_DEFAULT_BUS	2
#define CONFIG_SF_DEFAULT_CS	1
#define CONFIG_SF_DEFAULT_MODE	3


/*
 * USB Settings
 */
#define CONFIG_MUSB_HCD
#define CONFIG_USB_SC58X
#define CONFIG_MUSB_TIMEOUT 100000
#define MUSB_HW_VERSION2
#define CONFIG_USB_STORAGE

/*
 * SDH Settings
 */
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SC5XX_DWMMC
#define CONFIG_DWMMC
#define CONFIG_BOUNCE_BUFFER

/*
 * Env Storage Settings
 */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_OFFSET       0x10000
#define CONFIG_ENV_SIZE         0x2000
#define CONFIG_ENV_SECT_SIZE    0x10000
#define CONFIG_ENV_SPI_BUS 2
#define CONFIG_ENV_SPI_CS 1

/*
 * Misc Settings
 */
#define CONFIG_SYS_NO_FLASH
#define CONFIG_UART_CONSOLE	0
#define CONFIG_BAUDRATE		57600
#define CONFIG_UART4_SERIAL
#define CONFIG_LINUX_MEMSIZE	"224M"
#define CONFIG_LINUX_LPJ	595968
#define CONFIG_CMD_BOOTZ

#define CONFIG_BOOTCOMMAND	"run ramboot"
#define INITRAMADDR "0xC5000000"

/*
 * Nor Flash for SPL boot
 * 0x000000 - 0x010000 : SPL	(64KiB)
 * 0x010000 - 0x012000 : ENV	(8KiB)
 * 0x012000 - 0x092000 : U-Boot (512KiB)
 * 0x092000 - 0x0a0000 : DTB	(56KiB)
 * 0x0a0000 - 0x1500000: Linux Kernel - zImage (20MB)
 * 0x1500000 - 0x4000000: Linux Kernel - initramfs (43MB)
 */
/* Defines for SPL */
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_CONSOLE

#define CONFIG_SPL_LIBCOMMON_SUPPORT
/*#define CONFIG_SPL_LIBDISK_SUPPORT - sdcard/mmc*/
#define CONFIG_SPL_I2C_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_SPI_SUPPORT
#define CONFIG_SPL_SPI_FLASH_SUPPORT

#define CONFIG_SPL_SDRAM_BASE		0xc0000000
#define CONFIG_SPL_TEXT_BASE		CONFIG_SPL_SDRAM_BASE
#define CONFIG_SPL_MAX_SIZE			(64 * 1024)
#define CONFIG_SPL_BSS_START_ADDR	(CONFIG_SPL_TEXT_BASE + \
						CONFIG_SPL_MAX_SIZE)
#define CONFIG_SPL_BSS_MAX_SIZE		(32 * 1024)
#define CONFIG_SYS_SPL_MALLOC_START	(CONFIG_SPL_BSS_START_ADDR + \
						CONFIG_SPL_BSS_MAX_SIZE)
#define CONFIG_SYS_SPL_MALLOC_SIZE	(32 * 1024)
#define CONFIG_SPL_STACK_SIZE		(8 * 1024)
#define CONFIG_SPL_STACK			(CONFIG_SYS_SPL_MALLOC_START + \
						CONFIG_SYS_SPL_MALLOC_SIZE + \
						CONFIG_SPL_STACK_SIZE)

#define CONFIG_SPL_LDSCRIPT         "$(CPUDIR)/sc58x/u-boot-spl.lds"

/* SPL OS boot options */
#define CONFIG_CMD_SPL
#define CONFIG_SYS_SPL_ARGS_ADDR	0xcf8cc000 /* dtb + args load address(SDRAM) */

#define CONFIG_SYS_SPI_U_BOOT_OFFS  0x12000
#define CONFIG_SYS_SPI_ARGS_OFFS	0x92000 /* CONFIG_SYS_SPI_U_BOOT_OFFS + 0x80000 */
#define CONFIG_SYS_SPI_ARGS_SIZE	0xE000	/* 56KiB */
#define CONFIG_SYS_SPI_KERNEL_OFFS	0xa0000 /* CONFIG_SYS_SPI_ARGS_OFFS + CONFIG_SYS_SPI_ARGS_SIZE */
#define CONFIG_SYS_SPI_KERNEL_SIZE	0x1460000 /* 20MB */
#define CONFIG_SYS_SPI_INITRAM_OFFS	0x1500000 /* CONFIG_SYS_SPI_KERNEL_OFFS + CONFIG_SYS_SPI_KERNEL_SIZE */

#define CONFIG_SPL_OS_BOOT
#define CONFIG_SPL_SPI_LOAD
/* PB1-GPIO_PF0(gpio 80) is used as SPL_OS_BOOT_KEY */
#define SC5XX_SPL_OS_BOOT_KEY	GPIO_PF0

#include <configs/sc_adi_common.h>

#endif
