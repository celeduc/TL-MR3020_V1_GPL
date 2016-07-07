#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <config.h>
#include <version.h>
#include "ar7240_soc.h"

extern void ar7240_ddr_initial_config(uint32_t refresh);
extern int ar7240_ddr_find_size(void);

#ifdef CONFIG_HORNET_EMU
extern void ar7240_ddr_initial_config_for_fpga(void);
#endif

void
ar7240_usb_initial_config(void)
{
#ifndef CONFIG_HORNET_EMU
    ar7240_reg_wr_nf(AR7240_USB_PLL_CONFIG, 0x0a04081e);
    ar7240_reg_wr_nf(AR7240_USB_PLL_CONFIG, 0x0804081e);
#endif
}

void
ar7240_usb_otp_config(void)
{
    unsigned int addr, reg_val, reg_usb;
    int time_out, status, usb_valid;
    
    for (addr = 0xb8114014; ;addr -= 0x10) {
        status = 0;
        time_out = 20;
        
        reg_val = ar7240_reg_rd(addr);

        while ((time_out > 0) && (~status)) {
            if ((( ar7240_reg_rd(0xb8115f18)) & 0x7) == 0x4) {
                status = 1;
            } else {
                status = 0;
            }
            time_out--;
        }

        reg_val = ar7240_reg_rd(0xb8115f1c);
        if ((reg_val & 0x80) == 0x80){
            usb_valid = 1;
            reg_usb = reg_val & 0x000000ff;
        }

        if (addr == 0xb8114004) {
            break;
        }
    }

    if (usb_valid) {
        reg_val = ar7240_reg_rd(0xb8116c88);
        reg_val &= ~0x03f00000;
        reg_val |= (reg_usb & 0xf) << 22;
        ar7240_reg_wr(0xb8116c88, reg_val);
    }
}

#define	MY_WRITE(y, z)		((*((volatile u32*)(y))) = z)
#define	MY_READ(y)		(*((volatile u32*)(y)))
#define SETBITVAL(val, pos, bit)				\
    do { 							\
	ulong bitval = (bit) ? 0x1 : 0x0;			\
	(val) = ((val) & ~(0x1 << (pos))) | ( (bitval) << (pos));\
    } while (0)

void ar7240_all_led_on(void)
{
	ulong gpio;
	int index;
	gpio = MY_READ(0xb8040008);
#ifdef CONFIG_PID_MR302001
    SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, 17, 0);
#endif
#ifdef CONFIG_PID_WR74104
    SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, GPIO_SYS_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, 13, 1);
	SETBITVAL(gpio, 14, 1);
	SETBITVAL(gpio, 15, 1);
	SETBITVAL(gpio, 16, 1);
	SETBITVAL(gpio, 17, 0);
#endif
#ifdef CONFIG_PID_WR74302CN
	SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, GPIO_SYS_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 13, 1);
	SETBITVAL(gpio, 14, 1);
	SETBITVAL(gpio, 15, 1);
	SETBITVAL(gpio, 16, 1);
	SETBITVAL(gpio, 17, 0);
#endif
#ifdef CONFIG_PID_MR322002
	SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, GPIO_SYS_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 13, 1);
	SETBITVAL(gpio, 14, 1);
	SETBITVAL(gpio, 15, 1);
	SETBITVAL(gpio, 16, 1);
	SETBITVAL(gpio, 17, 0);
#endif
#ifdef CONFIG_PID_WR70301
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, GPIO_SYS_LED_ON);
#endif
#ifdef CONFIG_PID_MR11U02
#ifndef GPIO_SYS_LED_BIT
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 17, 0);
#endif
#endif 

#ifdef CONFIG_PID_TR86301
#ifndef GPIO_SYS_LED_BIT
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 17, 0);
#endif
#endif

#ifdef CONFIG_PID_TR86601
#ifndef GPIO_SYS_LED_BIT
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 17, 0);
#endif
#endif
#ifdef CONFIG_PID_WA70102
SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, GPIO_JUMPSTART_LED_ON);
SETBITVAL(gpio, GPIO_SYS_LED_BIT, GPIO_SYS_LED_ON);
SETBITVAL(gpio, GPIO_WLAN_LED_BIT, GPIO_WLAN_LED_ON);
SETBITVAL(gpio, 17, 0);
#endif

#ifdef GPIO_SLOW_ETH_LED
/*by zml 120207*/
    SETBITVAL(gpio, GPIO_SYS_LED_BIT, GPIO_SYS_LED_ON);
#endif

#ifdef CONFIG_PID_721002
    SETBITVAL(gpio, 17, 0);
    SETBITVAL(gpio, 0, 1);
    SETBITVAL(gpio, 1, 1);
    SETBITVAL(gpio, 27, 0);
    SETBITVAL(gpio, 26, 0);
#endif

	MY_WRITE(0xb8040008, gpio);
}

void ar7240_all_led_off(void)
{
	ulong gpio;
	int index;
	gpio = MY_READ(0xb8040008);
#ifdef CONFIG_PID_MR302001
	SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT,!GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, !GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, 17, 1);
#endif
#ifdef CONFIG_PID_WR74104
    	SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, !GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, !GPIO_SYS_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, 13, 0);
	SETBITVAL(gpio, 14, 0);
	SETBITVAL(gpio, 15, 0);
	SETBITVAL(gpio, 16, 0);
	SETBITVAL(gpio, 17, 1);
#endif
#ifdef CONFIG_PID_WR74302CN
	SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, !GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, !GPIO_SYS_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, !GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 13, 0);
	SETBITVAL(gpio, 14, 0);
	SETBITVAL(gpio, 15, 0);
	SETBITVAL(gpio, 16, 0);
	SETBITVAL(gpio, 17, 1);
#endif
#ifdef CONFIG_PID_MR322002
	SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, !GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, !GPIO_SYS_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, !GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 13, 0);
	SETBITVAL(gpio, 14, 0);
	SETBITVAL(gpio, 15, 0);
	SETBITVAL(gpio, 16, 0);
	SETBITVAL(gpio, 17, 1);
#endif
/*
#ifdef CONFIG_PID_WR70301
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, !GPIO_SYS_LED_ON);
#endif
*/
#ifdef CONFIG_PID_MR11U02
#ifndef GPIO_SYS_LED_BIT

	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, !GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 17, 1);
#endif
#endif

#ifdef CONFIG_PID_TR86301
#ifndef GPIO_SYS_LED_BIT
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, !GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 17, 1);
#endif
#endif


#ifdef CONFIG_PID_TR86601
#ifndef GPIO_SYS_LED_BIT

	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, GPIO_INTERNET_LED_BIT, !GPIO_INTERNET_LED_ON);
	SETBITVAL(gpio, 17, 1);
#endif
#endif

#ifdef CONFIG_PID_WA70102
    SETBITVAL(gpio, GPIO_JUMPSTART_LED_BIT, !GPIO_JUMPSTART_LED_ON);
	SETBITVAL(gpio, GPIO_SYS_LED_BIT, !GPIO_SYS_LED_ON);
	SETBITVAL(gpio, GPIO_WLAN_LED_BIT, !GPIO_WLAN_LED_ON);
	SETBITVAL(gpio, 17, 1);

#endif

#ifdef GPIO_SLOW_ETH_LED
    /*by zml 120207*/
    SETBITVAL(gpio, GPIO_SYS_LED_BIT, !GPIO_SYS_LED_ON);
#endif

#ifdef CONFIG_PID_721002
    SETBITVAL(gpio, 17, 1);
    SETBITVAL(gpio, 0, 0);
    SETBITVAL(gpio, 1, 0);
    SETBITVAL(gpio, 27, 1);
    SETBITVAL(gpio, 26, 1);
#endif

	MY_WRITE(0xb8040008, gpio);
}

void ar7240_gpio_config(void)
{
    /* Disable clock obs 
     * clk_obs1(gpio13/bit8),  clk_obs2(gpio14/bit9), clk_obs3(gpio15/bit10),
     * clk_obs4(gpio16/bit11), clk_obs5(gpio17/bit12)
     * clk_obs0(gpio1/bit19), 6(gpio11/bit20)
     */
    ar7240_reg_wr (AR7240_GPIO_FUNC, 
        (ar7240_reg_rd(AR7240_GPIO_FUNC) & ~((0x1f<<8)|(0x3<<19))));

    /* Enable eth Switch LEDs */
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) | (0x1f<<3)));
	
    /* Clear AR7240_GPIO_FUNC BIT2 to ensure that software can control LED5(GPIO16) and LED6(GPIO17)  */
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & ~(0x1<<2)));
	
    /* Set HORNET_BOOTSTRAP_STATUS BIT18 to ensure that software can control GPIO26 and GPIO27 */
    ar7240_reg_wr (HORNET_BOOTSTRAP_STATUS, (ar7240_reg_rd(HORNET_BOOTSTRAP_STATUS) | (0x1<<18)));

	/* Disable EJTAG functionality to enable GPIO 8, added by zcf, 20110608 */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) | 0x01));
#ifdef CONFIG_PID_MR302001
	/* set OE, added by zcf, 20110509 */
	ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc020001));
	/* Disable clock obs, added by zcf, 20110509 */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e07f));
#endif
#ifdef CONFIG_PID_WR74104
	/* set OE, added by zcf, 20110509 */
	ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc03e001));
	/* Disable clock obs, added by zcf, 20110509 */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e007));
#endif
#ifdef CONFIG_PID_WR74302CN
	/* set OE, added by zcf, 20110714 */
	ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc03e003));
	/* Disable clock obs, added by zcf, 20110509 */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e007));
#endif
#ifdef CONFIG_PID_MR322002
	/* set OE, added by zcf, 20110714 */
	ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc03e001));
	/* Disable clock obs, added by zcf, 20110509 */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e007));
#endif
#ifdef CONFIG_PID_WR70301
	/* set OE, added by zcf, 20110714 */
	ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc03e001));
#endif
#ifdef CONFIG_PID_MR11U02
    /* set OE, added by zml, 20111018 */
    ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc020001));
    /* Disable clock obs, added by zml, 20111018 */
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e07f));
#endif
#ifdef CONFIG_PID_TR86601
    /* set OE, added by zml, 20111018 */
ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc020001));
/* Disable clock obs, added by zml, 20111018 */
ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e07f));
#endif
#ifdef CONFIG_PID_TR86301
    /* set OE, added by zml, 20111018 */
    ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc020001));
    /* Disable clock obs, added by zml, 20111018 */
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e07f));
#endif
#ifdef CONFIG_PID_WA70102
	/* set OE, added by zml, 20111206 */
	ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc03e003));
	/* Disable clock obs, added by zml, 20111206 */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e007));
#endif

#ifdef GPIO_SLOW_ETH_LED
	/* set OE, added by zml, 120207 */
	ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | GPIO_SYS_LED_BIT));
	/* Disable clock obs, added by zml, 120207 */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e007));
#endif

#ifdef CONFIG_PID_721002
    /* set OE, added by zb,reference to config.721002 */
    ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0xc020003));
    /* Disable clock obs, added by zcf, 20110509 */
    ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e07f));
#endif

}

#if 0
/* make switch LED control independently modified by tiger 20091225 */
void ar7240_gpio_config()
{
	/* Disable clock obs */
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xffe7e0ff));
}
#endif

/* GPIO FUNCTION enable switch control LED added by tiger 20091225 */
void ar7240_gpio_sw_led()
{
	/* Enable eth Switch LEDs */
#ifdef CONFIG_K31
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) | 0xd8));
#else
	ar7240_reg_wr (AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) | 0xfa));
#endif
}

int
ar7240_mem_config(void)
{
#ifndef COMPRESSED_UBOOT
    unsigned int tap_val1, tap_val2;
#endif
#ifdef CONFIG_HORNET_EMU
    ar7240_ddr_initial_config_for_fpga();
#else
    //ar7240_ddr_initial_config(CFG_DDR_REFRESH_VAL);
#endif

/* Default tap values for starting the tap_init*/
    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL0, CFG_DDR_TAP0_VAL);
    ar7240_reg_wr (AR7240_DDR_TAP_CONTROL1, CFG_DDR_TAP1_VAL);

    ar7240_gpio_config();
    ar7240_all_led_off();

#ifndef COMPRESSED_UBOOT
#ifndef CONFIG_HORNET_EMU
    ar7240_ddr_tap_init();

    tap_val1 = ar7240_reg_rd(0xb800001c);
    tap_val2 = ar7240_reg_rd(0xb8000020);
    printf("#### TAP VALUE 1 = %x, 2 = %x\n",tap_val1, tap_val2);
#endif
#endif

    //ar7240_usb_initial_config();
    ar7240_usb_otp_config();
    
    hornet_ddr_tap_init();

    return (ar7240_ddr_find_size());
}

long int initdram(int board_type)
{
    return (ar7240_mem_config());
}

#ifdef COMPRESSED_UBOOT
int checkboard (char *board_string)
{
    strcpy(board_string, "AP121 (ar9330) U-boot");
    return 0;
}
#else
int checkboard (void)
{
    printf("AP121 (ar9330) U-boot\n");
    return 0;
}
#endif /* #ifdef COMPRESSED_UBOOT */

