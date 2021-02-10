/*
 * U-boot - Common configuration file for ADI SC serial board
 */

#ifndef __CONFIG_SC_ADI_COMMON_H
#define __CONFIG_SC_ADI_COMMON_H

#include <generated/autoconf.h>

/*
 * Command Settings
 */
#ifndef _CONFIG_CMD_DEFAULT_H
# include <config_cmd_default.h>
# ifdef ADI_CMDS_NETWORK
#  define CONFIG_CMD_DHCP
#  define CONFIG_BOOTP_SUBNETMASK
#  define CONFIG_BOOTP_GATEWAY
#  define CONFIG_BOOTP_DNS
#  define CONFIG_BOOTP_NTPSERVER
#  define CONFIG_KEEP_SERVERADDR
#  define CONFIG_CMD_DNS
#  define CONFIG_CMD_PING
#  define CONFIG_CMD_NET
#  ifdef CONFIG_MII
#   define CONFIG_CMD_MII
#  endif
# else
#  undef CONFIG_CMD_BOOTD
#  undef CONFIG_CMD_NET
#  undef CONFIG_CMD_NFS
# endif
# ifdef CONFIG_MMC
#  define CONFIG_CMD_MMC
#  define CONFIG_CMD_EXT2
#  define CONFIG_CMD_FAT
#  define CONFIG_DOS_PARTITION
# endif
# ifdef CONFIG_MUSB_HCD
#  define CONFIG_CMD_USB
#  define CONFIG_CMD_EXT2
#  define CONFIG_CMD_FAT
#  define CONFIG_DOS_PARTITION
# endif
# ifdef CONFIG_MMC_SPI
#  define CONFIG_CMD_MMC_SPI
# endif
# ifdef CONFIG_SPI_FLASH
#  define CONFIG_CMD_SF
# endif
# ifdef CONFIG_SYS_I2C
#  define CONFIG_CMD_I2C
# endif
# ifdef CONFIG_SYS_NO_FLASH
#  undef CONFIG_CMD_FLASH
#  undef CONFIG_CMD_IMLS
# endif
# define CONFIG_CMD_CACHE
# define CONFIG_CMD_ELF
# define CONFIG_CMD_GPIO
# define CONFIG_CMD_REGINFO
# define CONFIG_CMD_STRINGS
# define CONFIG_CMD_MEMTEST
# if defined(CONFIG_SC59X_CHAIN_BOOT) || defined(CONFIG_SC58X_CHAIN_BOOT) || defined(CONFIG_SC57X_CHAIN_BOOT)
#  undef CONFIG_CMD_SAVEENV
# endif
#if defined(CONFIG_SC58X) || defined(CONFIG_SC59X)
# define CONFIG_CMD_SC58X_OTP
#elif defined(CONFIG_SC57X)
# define CONFIG_CMD_SC57X_OTP
#endif
#endif

/* Commands */

#define CONFIG_CMDLINE_EDITING	1
#define CONFIG_AUTO_COMPLETE	1
#define CONFIG_LOADS_ECHO	1

/*
 * Debug Settings
 */
#define CONFIG_ENV_OVERWRITE	1
#define CONFIG_DEBUG_DUMP	1
#define CONFIG_KALLSYMS		1
#define CONFIG_PANIC_HANG	1


/*
 * Env Settings
 */
#ifndef CONFIG_BOOTDELAY
# if (CONFIG_SC_BOOT_MODE == SC_BOOT_UART)
#  define CONFIG_BOOTDELAY	-1
# else
#  define CONFIG_BOOTDELAY	5
# endif
#endif

#ifdef CONFIG_VIDEO
# define CONFIG_BOOTARGS_VIDEO "console=tty0 "
#else
# define CONFIG_BOOTARGS_VIDEO ""
#endif

#define CONFIG_BOOTARGS_ROOT_NAND "/dev/mtdblock2 rw"
#define CONFIG_BOOTARGS_ROOT_SDCARD    "/dev/mmcblk0p1 rw"

#define CONFIG_BOOTARGS_SDCARD	\
	"root=" CONFIG_BOOTARGS_ROOT_SDCARD " " \
	"rootfstype=ext2 " \
	"clkin_hz=" __stringify(CONFIG_CLKIN_HZ) " " \
	CONFIG_BOOTARGS_VIDEO \
	"earlyprintk=serial,uart0,57600 " \
	"console=ttySC" __stringify(CONFIG_UART_CONSOLE) "," \
			__stringify(CONFIG_BAUDRATE) " "\
	"mem=" CONFIG_LINUX_MEMSIZE

#define CONFIG_BOOTARGS_NFS	\
	"root=/dev/nfs rw " \
	"nfsroot=${serverip}:${rootpath},tcp,nfsvers=3 " \
	"clkin_hz=" __stringify(CONFIG_CLKIN_HZ) " " \
	CONFIG_BOOTARGS_VIDEO \
	"earlyprintk=serial,uart0,57600 " \
	"console=ttySC" __stringify(CONFIG_UART_CONSOLE) "," \
			__stringify(CONFIG_BAUDRATE) " "\
	"mem=" CONFIG_LINUX_MEMSIZE

#define CONFIG_BOOTARGS	\
	"root=" CONFIG_BOOTARGS_ROOT_NAND " " \
	"rootfstype=jffs2 " \
	"clkin_hz=" __stringify(CONFIG_CLKIN_HZ) " " \
	CONFIG_BOOTARGS_VIDEO \
	"earlyprintk=serial,uart0,57600 " \
	"console=ttySC" __stringify(CONFIG_UART_CONSOLE) "," \
			__stringify(CONFIG_BAUDRATE) " "\
		"mem=" CONFIG_LINUX_MEMSIZE

#if defined(CONFIG_SPL_OS_BOOT)
#define CONFIG_SPL_BOOTARGS	\
	"root=" CONFIG_BOOTARGS_ROOT_NAND " " \
	"rootfstype=jffs2 " \
	"clkin_hz=" __stringify(CONFIG_CLKIN_HZ) " " \
	CONFIG_BOOTARGS_VIDEO \
	"console=ttySC" __stringify(CONFIG_UART_CONSOLE) "," \
			__stringify(CONFIG_BAUDRATE) " "\
		"mem=" CONFIG_LINUX_MEMSIZE
# else
#  define CONFIG_SPL_BOOTARGS
# endif

#if defined(CONFIG_CMD_NET)
# define UBOOT_ENV_FILE "u-boot-" CONFIG_SYS_BOARD ".ldr"
# if (CONFIG_SC_BOOT_MODE == SC_BOOT_SPI_MASTER)
#  ifndef CONFIG_SPI_IMG_SIZE
#   define CONFIG_SPI_IMG_SIZE 0x80000
#  endif
#  define UBOOT_ENV_UPDATE \
		"sf probe " __stringify(CONFIG_SC_BOOT_SPI_BUS) ":" \
		__stringify(CONFIG_SC_BOOT_SPI_SSEL) " " \
		__stringify(CONFIG_ENV_SPI_MAX_HZ) ";" \
		"sf erase 0 " __stringify(CONFIG_SPI_IMG_SIZE) ";" \
		"sf write ${loadaddr} 0 ${filesize}"
# else
#  define UBOOT_ENV_UPDATE
# endif

# if defined(CONFIG_SPL_SPI_LOAD)
#  define UBOOT_SPL_UPDATE \
		"tftp ${loadaddr} u-boot-spl.ldr;" \
		"sf probe " __stringify(CONFIG_SC_BOOT_SPI_BUS) ":" \
		__stringify(CONFIG_SC_BOOT_SPI_SSEL) " " \
		__stringify(CONFIG_ENV_SPI_MAX_HZ) ";" \
		"sf erase " __stringify(CONFIG_ENV_OFFSET) " " \
		__stringify(CONFIG_ENV_SIZE) ";" \
		"sf erase 0 +${filesize};" \
		"sf write ${loadaddr} 0 ${filesize};" \
		"tftp ${loadaddr} u-boot-" CONFIG_SYS_BOARD ".bin;" \
		"sf erase " __stringify(CONFIG_SYS_SPI_U_BOOT_OFFS) " " \
		"+${filesize};" \
		"sf write ${loadaddr} " __stringify(CONFIG_SYS_SPI_U_BOOT_OFFS) " " \
		"${filesize};"
#  if defined(CONFIG_SPL_OS_BOOT)
#   define UBOOT_SPL_EXPORT \
		"tftp ${loadaddr} ${ramfile};" \
		"tftp ${dtbaddr} ${dtbfile};" \
		"sf probe " __stringify(CONFIG_SC_BOOT_SPI_BUS) ":" \
		__stringify(CONFIG_SC_BOOT_SPI_SSEL) " " \
		__stringify(CONFIG_ENV_SPI_MAX_HZ) ";" \
		"sf erase " __stringify(CONFIG_SYS_SPI_ARGS_OFFS) " " \
		__stringify(CONFIG_SYS_SPI_ARGS_SIZE) ";" \
		"spl export fdt ${loadaddr} - ${dtbaddr};"
#   define KERNEL_IMAGE_UPDATE \
		"tftp ${loadaddr} ${ramfile};" \
		"sf probe " __stringify(CONFIG_SC_BOOT_SPI_BUS) ":" \
		__stringify(CONFIG_SC_BOOT_SPI_SSEL) " " \
		__stringify(CONFIG_ENV_SPI_MAX_HZ) ";" \
		"sf erase " __stringify(CONFIG_SYS_SPI_KERNEL_OFFS) " +${filesize};" \
		"sf write ${loadaddr} " __stringify(CONFIG_SYS_SPI_KERNEL_OFFS) " ${filesize};" \
		"tftp ${dtbaddr} ${dtbfile};" \
		"sf erase " __stringify(CONFIG_SYS_SPI_DTB_OFFS) " +${filesize};" \
		"sf write ${dtbaddr} " __stringify(CONFIG_SYS_SPI_DTB_OFFS) " ${filesize};"


#  else
#   define UBOOT_SPL_EXPORT
#  endif /* defined(CONFIG_SPL_OS_BOOT)  */
# else
#  define UBOOT_SPL_UPDATE
#  define UBOOT_SPL_EXPORT
# endif /* defined(CONFIG_SPL_SPI_LOAD)  */
# ifdef CONFIG_NETCONSOLE
#  define NETCONSOLE_ENV \
	"nc=" \
		"set ncip ${serverip};" \
		"set stdin nc;" \
		"set stdout nc;" \
		"set stderr nc" \
		"\0"
# else
#  define NETCONSOLE_ENV
# endif
# define NETWORK_ENV_SETTINGS \
	NETCONSOLE_ENV \
	\
	"ubootfile=" UBOOT_ENV_FILE "\0" \
	"update=" \
		"tftp ${loadaddr} ${ubootfile};" \
		UBOOT_ENV_UPDATE \
		"\0" \
	"addip=set bootargs ${bootargs} " \
		"ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:" \
		   "${hostname}:eth0:off" \
		"\0" \
	\
	"ramfile=uImage\0" \
	"initramfile=ramdisk.cpio.gz.u-boot\0" \
	"initramaddr=" INITRAMADDR "\0" \
	"dtbfile=" CONFIG_DTBNAME "\0" \
	"dtbaddr=" CONFIG_DTBLOADADDR "\0" \
	"sdcardargs=set bootargs " CONFIG_BOOTARGS_SDCARD "\0" \
	"ramargs=set bootargs " CONFIG_BOOTARGS "\0" \
	"nfsargs=set bootargs " CONFIG_BOOTARGS_NFS "\0" \
	"ramboot_emmc=" \
		"mmc rescan;" \
		"mmc dev 0 0;" \
		"ext2load mmc 0:1 ${loadaddr} /boot/${ramfile};" \
		"ext2load mmc 0:1 ${dtbaddr} /boot/${dtbfile};" \
		"ext2load mmc 0:1 ${initramaddr} /boot/${initramfile};" \
		"run sdcardargs;" \
		"run addip;" \
		"bootm ${loadaddr} ${initramaddr} ${dtbaddr}" \
		"\0" \
	\
	"ramboot=" \
		"tftp ${loadaddr} ${ramfile};" \
		"tftp ${dtbaddr} ${dtbfile};" \
		"run ramargs;" \
		"run addip;" \
		"bootm ${loadaddr} - ${dtbaddr}" \
		"\0" \
	\
	"norboot=" \
		"sf probe " __stringify(CONFIG_SC_BOOT_SPI_BUS) ":" \
		__stringify(CONFIG_SC_BOOT_SPI_SSEL) " " \
		__stringify(CONFIG_ENV_SPI_MAX_HZ) ";" \
		"sf read  ${loadaddr} " __stringify(CONFIG_SYS_SPI_KERNEL_OFFS) " " "0xa00000;" \
		"sf read  ${dtbaddr} " __stringify(CONFIG_SYS_SPI_DTB_OFFS) " " __stringify(CONFIG_SYS_SPI_DTB_SIZE) ";" \
		"run ramargs;" \
		"run addip;" \
		"bootm ${loadaddr} - ${dtbaddr}" \
		"\0" \
	\
	"sdcardboot=" \
		"mmc rescan;" \
		"mmc dev 0 0;" \
		"ext2load mmc 0:1 ${loadaddr} /boot/${ramfile};" \
		"ext2load mmc 0:1 ${dtbaddr} /boot/${dtbfile};" \
		"run sdcardargs;" \
		"bootm ${loadaddr} - ${dtbaddr}" \
		"\0" \
	\
	"nfsfile=uImage\0" \
	"nfsboot=" \
		"tftp ${loadaddr} ${nfsfile};" \
		"tftp ${dtbaddr} ${dtbfile};" \
		"run nfsargs;" \
		"run addip;" \
		"bootm ${loadaddr} - ${dtbaddr}" \
		"\0" \
	"kupdate=" \
		KERNEL_IMAGE_UPDATE \
		"\0" \
	"splargs=set bootargs " CONFIG_SPL_BOOTARGS " " \
		"ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:" \
		"${hostname}:eth0:off " \
		"quiet lpj="  __stringify(CONFIG_LINUX_LPJ) \
		"\0" \
	"splexport=" \
		"run splargs;" \
		UBOOT_SPL_EXPORT \
		"\0" \
	"splargs_update=" \
		"sf write " __stringify(CONFIG_SYS_SPL_ARGS_ADDR) " " \
		__stringify(CONFIG_SYS_SPI_ARGS_OFFS) " " \
		__stringify(CONFIG_SYS_SPI_ARGS_SIZE) \
		"\0" \
	"splupdate=" \
		UBOOT_SPL_UPDATE \
		"\0"
#else
# define NETWORK_ENV_SETTINGS
#endif

#define CONFIG_ENV_OVERWRITE    1

#ifndef BOARD_ENV_SETTINGS
# define BOARD_ENV_SETTINGS
#endif

#if ! defined(CONFIG_SC59X)
	#define ADI_ENV_SETTINGS "\0"
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	NETWORK_ENV_SETTINGS \
	BOARD_ENV_SETTINGS \
	ADI_ENV_SETTINGS

/*
 * Boot Paramter Settings
 */
#define CONFIG_CMDLINE_TAG              1       /* enable passing of ATAGs */
#define CONFIG_OF_LIBFDT                /* Device Tree support */
#define CONFIG_SETUP_MEMORY_TAGS        1
#define CONFIG_SYS_GENERIC_BOARD

/*
 * Network Settings
 */
#ifdef CONFIG_CMD_NET
# define CONFIG_NETMASK         255.255.255.0
# ifndef CONFIG_IPADDR
#  define CONFIG_IPADDR         192.168.0.15
#  define CONFIG_GATEWAYIP      192.168.0.1
#  define CONFIG_SERVERIP       192.168.0.2
# endif
# ifndef CONFIG_ROOTPATH
#  define CONFIG_ROOTPATH       "/romfs"
# endif
# ifdef CONFIG_CMD_DHCP
#  ifndef CONFIG_SYS_AUTOLOAD
#   define CONFIG_SYS_AUTOLOAD "no"
#  endif
# endif
# define CONFIG_IP_DEFRAG
# define CONFIG_NET_RETRY_COUNT 20
#endif


/*
 * I2C Settings
 */
#ifdef CONFIG_SYS_I2C
# ifndef CONFIG_SYS_I2C_SPEED
#  define CONFIG_SYS_I2C_SPEED 100000
# endif
# ifndef CONFIG_SYS_I2C_SLAVE
#  define CONFIG_SYS_I2C_SLAVE 0
# endif
#endif


/*
 * SPI Settings
 */
#ifdef CONFIG_SPI_FLASH_ALL
# define CONFIG_SPI_FLASH_ATMEL
# define CONFIG_SPI_FLASH_EON
# define CONFIG_SPI_FLASH_MACRONIX
# define CONFIG_SPI_FLASH_SPANSION
# define CONFIG_SPI_FLASH_SST
# define CONFIG_SPI_FLASH_STMICRO
# define CONFIG_SPI_FLASH_WINBOND
#endif
#ifndef CONFIG_SPI_MM_BASE
# define CONFIG_SPI_MM_BASE      0x60000000
#endif
#ifndef CONFIG_SPI_MM_SIZE
# define CONFIG_SPI_MM_SIZE      0x20000000
#endif


/*
 * Env Storage Settings
 */
#ifndef CONFIG_SC59X
	#define CONFIG_ENV_OFFSET       0x10000
	#define CONFIG_ENV_SIZE         0x2000
	#define CONFIG_ENV_SECT_SIZE    0x10000
	/* We need envcrc to embed the env into LDRs */
	#ifdef CONFIG_ENV_IS_EMBEDDED_IN_LDR
	# define CONFIG_BUILD_ENVCRC
	#endif
#endif

/*
 * Misc Settings
 */
#define CONFIG_SYS_HZ			1000
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser */
#define CONFIG_SYS_PROMPT		"sc # "
#define CONFIG_SYS_CBSIZE		512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_CMD_MEMORY
#define CONFIG_MISC_INIT_R
#define CONFIG_ADI_GPIO2
#define CONFIG_SOFT_SWITCH
#define CONFIG_ARCH_HEADER_IN_MACH
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_BUILD_LDR

#ifdef CONFIG_SC5XX_DWMMC
# ifndef CONFIG_SC5XX_BUS_WIDTH
#  define CONFIG_SC5XX_BUS_WIDTH   4
# endif
#endif

/*
#define CONFIG_HW_WATCHDOG
#define CONFIG_ADI_WATCHDOG
#define CONFIG_WATCHDOG_TIMEOUT_MSECS 10000
*/

#define CONFIG_RSA

#endif
