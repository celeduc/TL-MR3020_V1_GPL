
/*
 * Prom setup file for ar7240
 */

#include <linux/init.h>
//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/bootmem.h>

#include <asm/bootinfo.h>
#include <asm/addrspace.h>

#include "ar7240.h"

int __ath_flash_size;

void __init prom_init(void)
{
    int memsz = 0x2000000, argc = fw_arg0, i;
	char **arg = (char**) fw_arg1;

    printk ("flash_size passed from bootloader = %d\n", (int)fw_arg3);
    __ath_flash_size = fw_arg3;

/* We need the default one in .config so we dont need to change redboot --- lsz 071228 */
#if 0
	
	/* 
     * if user passes kernel args, ignore the default one 
     */
	if (argc > 1) {
		arcs_cmdline[0] = '\0';

        for (i = 1; i < argc; i++) 
            printk("arg %d: %s\n", i, arg[i]);

        /* 
         * arg[0] is "g", the rest is boot parameters 
         */
        for (i = 1; i < argc; i++) {
            if (strlen(arcs_cmdline) + strlen(arg[i] + 1)
                >= sizeof(arcs_cmdline))
                break;
            strcat(arcs_cmdline, arg[i]);
            strcat(arcs_cmdline, " ");
        }
    }
#endif

	/*  
	sprintf(arcs_cmdline, "%s",
		"console=ttyS0,115200 root=31:02 rootfstype=squashfs init=/sbin/init mtdparts=ar7240-nor0:128k(u-boot),1024k(uImage),2816k(rootfs),64k(NVRAM),64k(ART) mem=32M");
	*/

    //mips_machgroup = MACH_GROUP_AR7240;
    mips_machtype  = MACH_ATHEROS_AR7240;

    /*
     * By default, use all available memory.  You can override this
     * to use, say, 8MB by specifying "mem=8M" as an argument on the
     * linux bootup command line.
     */
    add_memory_region(0, memsz, BOOT_MEM_RAM);
}

void __init prom_free_prom_memory(void)
{
}





