/*
* documentation (author: zhaochunfeng@tp-link.net)
* we have extracted all dependent macros into config.$PID in build/scripts/spec/ so that we can produce one product
* by compiling with specific command, e.g. make kernel_build BOARD_TYPE=ap121 PID=302001, etc.
*/

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

/*#include <linux/config.h>*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/proc_fs.h>
/*#include <linux/miscdevice.h>*/
#include <asm/types.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/system.h>

#include <linux/mtd/mtd.h>
#include <linux/cdev.h>
#include <linux/irqreturn.h>
#include <linux/delay.h>

#include "ar7240.h"

/*
 * IOCTL Command Codes
 */

#define AR7240_GPIO_IOCTL_BASE			0x01
#define AR7240_GPIO_IOCTL_CMD1      	AR7240_GPIO_IOCTL_BASE
#define AR7240_GPIO_IOCTL_CMD2      	AR7240_GPIO_IOCTL_BASE + 0x01
#define AR7240_GPIO_IOCTL_CMD3      	AR7240_GPIO_IOCTL_BASE + 0x02
#define AR7240_GPIO_IOCTL_CMD4      	AR7240_GPIO_IOCTL_BASE + 0x03
#define AR7240_GPIO_IOCTL_CMD5      	AR7240_GPIO_IOCTL_BASE + 0x04
#define AR7240_GPIO_IOCTL_MAX			AR7240_GPIO_IOCTL_CMD5

#define AR7240_GPIO_MAGIC 				0xB2
#define	AR7240_GPIO_BTN_READ			_IOR(AR7240_GPIO_MAGIC, AR7240_GPIO_IOCTL_CMD1, int)
#define	AR7240_GPIO_LED_READ			_IOR(AR7240_GPIO_MAGIC, AR7240_GPIO_IOCTL_CMD2, int)
#define	AR7240_GPIO_LED_WRITE			_IOW(AR7240_GPIO_MAGIC, AR7240_GPIO_IOCTL_CMD3, int)

#define AR7240_GPIO_USB_LED1_WRITE		_IOW(AR7240_GPIO_MAGIC, AR7240_GPIO_IOCTL_CMD4, int)
#define	AR7240_GPIO_WLANBTN_READ			_IOR(AR7240_GPIO_MAGIC, AR7240_GPIO_IOCTL_CMD5, int)

#define gpio_major      				238
#define gpio_minor      				0

/*
 * GPIO assignment
 */

/* added by zcf */
#define CONFIG_NO_ETHSW		1

//#define GPIO_USB_LED0		0	/* controled by usb EHCI currently not used */
//#define GPIO_USB_LED1		8	/* usb connect or not, controled by software */

#ifdef GPIO_RESET_FAC_BIT
#define RST_DFT_GPIO		GPIO_RESET_FAC_BIT	/* reset default */
#define RST_HOLD_TIME	GPIO_FAC_RST_HOLD_TIME	/* How long the user must press the button before Router rst */
#endif

#ifdef GPIO_SYS_LED_BIT
#define SYS_LED_GPIO		GPIO_SYS_LED_BIT	/* system led 	*/
#define SYS_LED_ON		GPIO_SYS_LED_ON
#define SYS_LED_OFF		(!GPIO_SYS_LED_ON)
#endif

#ifdef GPIO_JUMPSTART_SW_BIT
#define JUMPSTART_GPIO		GPIO_JUMPSTART_SW_BIT	/* wireless jumpstart */
#endif

#ifdef GPIO_INTERNET_LED_BIT
//#define INTERNET_LED_GPIO	GPIO_INTERNET_LED_BIT
#define AP_USB_LED_GPIO     GPIO_INTERNET_LED_BIT	         /* USB LED */
#define USB_LED_OFF         (!GPIO_INTERNET_LED_ON)       /* USB LED's value when off */
#define USB_LED_ON          GPIO_INTERNET_LED_ON          /* USB LED's value when on */
#endif

#ifdef GPIO_JUMPSTART_LED_BIT
#define TRICOLOR_LED_GREEN_PIN			GPIO_JUMPSTART_LED_BIT
#define ON		GPIO_JUMPSTART_LED_ON
#define OFF		(!GPIO_JUMPSTART_LED_ON)
#endif

#ifdef GPIO_WLAN_LED_BIT
#define WLAN_LED_GPIO			GPIO_WLAN_LED_BIT
#define WLAN_LED_ON				GPIO_WLAN_LED_ON
#define WLAN_LED_OFF				(!GPIO_WLAN_LED_ON)
#endif

#ifdef GPIO_USB_POWER_SUPPORT
#define SYS_USB_POWER_GPIO 	GPIO_USB_POWER_SUPPORT
#define USB_POWER_ON		GPIO_USB_POWER_ON
#define USB_POWER_OFF		(!GPIO_USB_POWER_ON)
#endif

#ifdef GPIO_WLAN_BTN_BIT
#define WLAN_BTN_BIT        GPIO_WLAN_BTN_BIT
#endif

#ifdef GPIO_WLAN_HOLD_TIME
#define WLAN_HOLD_TIME GPIO_WLAN_HOLD_TIME
#endif

/*added by zb for rssi led*/
#ifdef GPIO_RSSI_LED_SUPPORT
#define RSSI1_LED_BIT   0
#define RSSI2_LED_BIT   1
#define RSSI3_LED_BIT   27
#define RSSI4_LED_BIT   26

#define RSSI1_LED_ON  1
#define RSSI2_LED_ON  1
#define RSSI3_LED_ON  0
#define RSSI4_LED_ON  0

#define RSSI1_LED_OFF  (!RSSI1_LED_ON)
#define RSSI2_LED_OFF  (!RSSI2_LED_ON)
#define RSSI3_LED_OFF  (!RSSI3_LED_ON)
#define RSSI4_LED_OFF  (!RSSI4_LED_ON)
#endif

/*by zml 2011 12 09*/

int counter = 0;
int wlan_counter=0;
int jiff_when_press = 0;
int bBlockWps = 1;
struct timer_list rst_timer;
/* added by ZCF */
struct timer_list wps_timer;
struct timer_list sysMode_timer;
/*added by zml 2011 12 09*/
#ifdef WLAN_BTN_BIT
struct timer_list wlan_timer;
#endif

/* control params for reset button reuse, by zjg, 13Apr10 */
//static int l_bMultiUseResetButton		=	0;
static int l_bWaitForQss				= 	1;

/* 0: internet led off; 1: on for 3G; 2: on for WISP; 3: on for wired connection */
int g_internetLedStatus = 0;
EXPORT_SYMBOL(g_internetLedStatus);

int g_internetLedPin = -1;
EXPORT_SYMBOL(g_internetLedPin);

#ifdef GPIO_INTERNET_LED_BIT
int g_usbPowerOn= USB_LED_ON;
#else
int g_usbPowerOn= -1;
#endif
EXPORT_SYMBOL(g_usbPowerOn);


/* in WISP mode, if wlan is on, then g_wlanLedStatus = 1, otherwise 0 */
int g_wlanLedStatus = 0;
EXPORT_SYMBOL(g_wlanLedStatus);

int g_sysMode = 0;
EXPORT_SYMBOL(g_sysMode);

int g_wispMode = SYSTEM_MODE_APC_ROUTER;
EXPORT_SYMBOL(g_wispMode);

int g_isSoftSysModeEnable = 0;

/*
 * GPIO interrupt stuff
 */
typedef enum {
    INT_TYPE_EDGE,
    INT_TYPE_LEVEL,
}ar7240_gpio_int_type_t;

typedef enum {
    INT_POL_ACTIVE_LOW,
    INT_POL_ACTIVE_HIGH,
}ar7240_gpio_int_pol_t;


/* 
** Simple Config stuff
*/
#if 0
#if !defined(IRQ_NONE)
#define IRQ_NONE
#define IRQ_HANDLED
#endif /* !defined(IRQ_NONE) */
#endif


/*changed by hujia.*/
typedef irqreturn_t(*sc_callback_t)(int, void *, struct pt_regs *, void *);

static sc_callback_t registered_cb = NULL;
static void *cb_arg;
/*add by hujia.*/
static void *cb_pushtime;
/*end add.*/
static int ignore_pushbutton = 1;
static struct proc_dir_entry *simple_config_entry = NULL;
static struct proc_dir_entry *simulate_push_button_entry = NULL;
static struct proc_dir_entry *tricolor_led_entry = NULL;
static struct proc_dir_entry *system_mode_entry = NULL;
static struct proc_dir_entry *internet_led_status_entry = NULL;
static struct proc_dir_entry *wlan_led_entry = NULL;
static struct proc_dir_entry *usb_power_entry = NULL;
#ifdef GPIO_RSSI_LED_SUPPORT
static struct proc_dir_entry *rssi_led_entry = NULL;
#endif


void ar7240_gpio_config_int(int gpio, 
                       ar7240_gpio_int_type_t type,
                       ar7240_gpio_int_pol_t polarity)
{
    u32 val;

    /*
     * allow edge sensitive/rising edge too
     */
    if (type == INT_TYPE_LEVEL) 
    {
        /* level sensitive */
        ar7240_reg_rmw_set(AR7240_GPIO_INT_TYPE, (1 << gpio));
    }
    else 
    {
       /* edge triggered */
       val = ar7240_reg_rd(AR7240_GPIO_INT_TYPE);
       val &= ~(1 << gpio);
       ar7240_reg_wr(AR7240_GPIO_INT_TYPE, val);
    }

    if (polarity == INT_POL_ACTIVE_HIGH) 
    {
        ar7240_reg_rmw_set (AR7240_GPIO_INT_POLARITY, (1 << gpio));
    }
    else 
    {
       val = ar7240_reg_rd(AR7240_GPIO_INT_POLARITY);
       val &= ~(1 << gpio);
       ar7240_reg_wr(AR7240_GPIO_INT_POLARITY, val);
    }

    ar7240_reg_rmw_set(AR7240_GPIO_INT_ENABLE, (1 << gpio));
}

void
ar7240_gpio_config_output(int gpio)
{
    ar7240_reg_rmw_set(AR7240_GPIO_OE, (1 << gpio));
}

void
ar7240_gpio_config_input(int gpio)
{
    ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1 << gpio));
}

void
ar7240_gpio_out_val(int gpio, int val)
{
    if (val & 0x1) {
        ar7240_reg_rmw_set(AR7240_GPIO_OUT, (1 << gpio));
    }
    else {
        ar7240_reg_rmw_clear(AR7240_GPIO_OUT, (1 << gpio));
    }
}

EXPORT_SYMBOL(ar7240_gpio_out_val);

int
ar7240_gpio_in_val(int gpio)
{
    return((1 << gpio) & (ar7240_reg_rd(AR7240_GPIO_IN)));
}

static void
ar7240_gpio_intr_enable(unsigned int irq)
{
    ar7240_reg_rmw_set(AR7240_GPIO_INT_MASK, 
                      (1 << (irq - AR7240_GPIO_IRQ_BASE)));
}

static void
ar7240_gpio_intr_disable(unsigned int irq)
{
    ar7240_reg_rmw_clear(AR7240_GPIO_INT_MASK, 
                        (1 << (irq - AR7240_GPIO_IRQ_BASE)));
}

static unsigned int
ar7240_gpio_intr_startup(unsigned int irq)
{
	ar7240_gpio_intr_enable(irq);
	return 0;
}

static void
ar7240_gpio_intr_shutdown(unsigned int irq)
{
	ar7240_gpio_intr_disable(irq);
}

static void
ar7240_gpio_intr_ack(unsigned int irq)
{
	ar7240_gpio_intr_disable(irq);
}

static void
ar7240_gpio_intr_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
		ar7240_gpio_intr_enable(irq);
}

static void
ar7240_gpio_intr_set_affinity(unsigned int irq, cpumask_t mask)
{
	/* 
     * Only 1 CPU; ignore affinity request
     */
}

struct irq_chip /* hw_interrupt_type */ ar7240_gpio_intr_controller = {
	.name		= "AR7240 GPIO",
	.startup	= ar7240_gpio_intr_startup,
	.shutdown	= ar7240_gpio_intr_shutdown,
	.enable		= ar7240_gpio_intr_enable,
	.disable	= ar7240_gpio_intr_disable,
	.ack		= ar7240_gpio_intr_ack,
	.end		= ar7240_gpio_intr_end,
	.eoi		= ar7240_gpio_intr_end,
	.set_affinity	= ar7240_gpio_intr_set_affinity,
};

void
ar7240_gpio_irq_init(int irq_base)
{
	int i;

	for (i = irq_base; i < irq_base + AR7240_GPIO_IRQ_COUNT; i++) {
		irq_desc[i].status  = IRQ_DISABLED;
		irq_desc[i].action  = NULL;
		irq_desc[i].depth   = 1;
		//irq_desc[i].chip = &ar7240_gpio_intr_controller;
		set_irq_chip_and_handler(i, &ar7240_gpio_intr_controller,
					 handle_percpu_irq);
	}
}

/* 
 *  USB GPIO control
 */

void turn_on_internet_led(void)
{
#ifdef AP_USB_LED_GPIO	
    ar7240_gpio_out_val(AP_USB_LED_GPIO, USB_LED_ON);
#endif
}

EXPORT_SYMBOL(turn_on_internet_led);

void turn_off_internet_led(void)
{
#ifdef AP_USB_LED_GPIO
    ar7240_gpio_out_val(AP_USB_LED_GPIO, USB_LED_OFF);
#endif
}

EXPORT_SYMBOL(turn_off_internet_led);

/*changed by hujia.*/
/*void register_simple_config_callback (void *callback, void *arg)*/
void register_simple_config_callback (void *callback, void *arg, void *arg2)
{
    registered_cb = (sc_callback_t) callback;
    cb_arg = arg;
    /* add by hujia.*/
    cb_pushtime=arg2;
    /*end add.*/
}
EXPORT_SYMBOL(register_simple_config_callback);

void unregister_simple_config_callback (void)
{
    registered_cb = NULL;
    cb_arg = NULL;
}
EXPORT_SYMBOL(unregister_simple_config_callback);

/*
 * Irq for front panel SW jumpstart switch
 * Connected to XSCALE through GPIO4
 */
static int jumpstart_cpl = 0;
struct pt_regs *jumpstart_pt_reg = NULL;
static int intr_mode = 0;

#ifndef JUMPSTART_RST_MULTIPLEXED
#ifdef JUMPSTART_GPIO
irqreturn_t jumpstart_irq(int cpl, void *dev_id, struct pt_regs *regs)
{
    if (ignore_pushbutton) 
    {
        ar7240_gpio_config_int (JUMPSTART_GPIO,INT_TYPE_LEVEL,
                                INT_POL_ACTIVE_HIGH);
        ignore_pushbutton = 0;        
		if (intr_mode)
		{
			jumpstart_cpl = cpl;
			jumpstart_pt_reg = regs;
			mod_timer(&wps_timer, jiffies + RST_HOLD_TIME * HZ);
		}
        
        return IRQ_HANDLED;
    }
    
    ar7240_gpio_config_int (JUMPSTART_GPIO,INT_TYPE_LEVEL,INT_POL_ACTIVE_LOW);
    ignore_pushbutton = 1;

    if (registered_cb && !bBlockWps && !intr_mode) {
    	printk ("Jumpstart button pressed.\n");
        return registered_cb (cpl, cb_arg, regs, NULL);
    }

    return IRQ_HANDLED;
}
void check_wps(unsigned long nothing)
{
	if (!ignore_pushbutton)
	{
		if (registered_cb && !bBlockWps) 
		{
         	registered_cb (jumpstart_cpl, cb_arg, jumpstart_pt_reg, NULL);
		 	jumpstart_cpl = 0;
		 	jumpstart_pt_reg = NULL;
		 	return;
    	}
	}
}
#endif
#endif



#ifdef WLAN_BTN_BIT
int ignore_wlanbutton = 0;
int wlan_open = 1;
irqreturn_t wlanbit_irq(int cpl, void *dev_id, struct pt_regs *regs)
{
    if (ignore_wlanbutton) 
	{
        ar7240_gpio_config_int(WLAN_BTN_BIT, INT_TYPE_LEVEL, INT_POL_ACTIVE_LOW);
        ignore_wlanbutton = 0;

		mod_timer(&wlan_timer, jiffies + WLAN_HOLD_TIME * HZ);

        return IRQ_HANDLED;
            }

    ar7240_gpio_config_int (WLAN_BTN_BIT, INT_TYPE_LEVEL, INT_POL_ACTIVE_HIGH);
    ignore_wlanbutton = 1;
    return IRQ_HANDLED;   
}
void check_wlan(unsigned long nothing)
{
	if (!ignore_wlanbutton)
	{
        wlan_counter ++;
    }
}

#endif


static int ignore_rstbutton = 1;

/* irq handler for reset button */
irqreturn_t rst_irq(int cpl, void *dev_id, struct pt_regs *regs)
{
    if (ignore_rstbutton) 
	{
        ar7240_gpio_config_int(RST_DFT_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_LOW);
        ignore_rstbutton = 0;

		mod_timer(&rst_timer, jiffies + RST_HOLD_TIME * HZ);

        return IRQ_HANDLED;
            }

    ar7240_gpio_config_int (RST_DFT_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_HIGH);
    ignore_rstbutton = 1;

#ifdef JUMPSTART_RST_MULTIPLEXED
	/* mark reset status, by zjg, 12Apr10 */
    if (registered_cb && l_bWaitForQss)
	{
		printk("Wps button pressed.\n");
        return registered_cb (cpl, cb_arg, regs, cb_pushtime);
    }
#endif
            return IRQ_HANDLED;
}

void check_rst(unsigned long nothing)
{
	if (!ignore_rstbutton)
	{
		printk ("restoring factory default...\n");
    		counter ++;

		/* to mark reset status, forbid QSS, added by zjg, 12Apr10 */
		l_bWaitForQss	= 0;
    	}
}

static int push_button_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
    return 0;
}

static int push_button_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
    if (registered_cb) {
		/*changed by hujia.*/
		/* registered_cb (0, cb_arg, 0);*/
		registered_cb (0, cb_arg, 0,NULL);
    }
    return count;
}

static int internet_led_status_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
    return sprintf (page, "%d\n", g_internetLedStatus);
}

/*
* buf: 0 means not connected to internet; 1 means 3G connected; 2 means wisp connected; 3 means wired connected.
*/
static int internet_led_status_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
    u_int32_t val;

	if (sscanf(buf, "%d", &val) != 1)
        return -EINVAL;

	if ((val < 0) || (val > 3))
		return -EINVAL;

	g_internetLedStatus = val;

	if (g_internetLedStatus)
	{
		turn_on_internet_led();
	}
	else
	{
		turn_off_internet_led();
	}

	return count;
}
#ifdef GPIO_USB_POWER_SUPPORT
/*added by ZQQ,10.06.02*/
static int usb_power_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
    return 0;
}

static int usb_power_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
    u_int32_t val = 0;

	if (sscanf(buf, "%d", &val) != 1)
        return -EINVAL;

	if ((val < 0) || (val > 1))
		return -EINVAL;

	printk("%s %d: write gpio:value = %d\r\n",__FUNCTION__,__LINE__,val);

	// Value 1 is for openning usb power in user space, and
	// USB_POWER_ON is for opening usb power in kernel space.
	// Whether USB_POWER_ON is 1 or 0, the method to open or close usb power
	// is always like this:
	// echo 1 > /proc/simple_config/usb_power (open usb power)
	// echo 0 > /proc/simple_config/usb_power (close usb power)
	// So that applications in user space don't need to change src code if
	// USB_POWER_ON is 0
	if (1 == val)
	{
		ar7240_gpio_out_val(SYS_USB_POWER_GPIO, USB_POWER_ON);
	}
	else
	{
		ar7240_gpio_out_val(SYS_USB_POWER_GPIO, USB_POWER_OFF);
	}
	
	return count;
}
/*end added */
#endif

typedef enum {
        LED_STATE_OFF   =       0,
        LED_STATE_GREEN =       1,
        LED_STATE_YELLOW =      2,
        LED_STATE_ORANGE =      3,
        LED_STATE_MAX =         4
} led_state_e;

static led_state_e gpio_tricolorled = LED_STATE_OFF;

/*
 * by dyf, on 27Sept2012
 * WISP led should be related to WISP config, rather not certain specific PID.
 */
#ifdef MODE_APC_ROUTER
static int gpio_wlan_status_read (char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
	return sprintf(page, "%d\n", g_wlanLedStatus);
}
static int gpio_wlan_status_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
	u_int32_t val = 0;

	if (sscanf(buf, "%d", &val) != 1)
        return -EINVAL;

	if ((val < 0) || (val > 1))
		return -EINVAL;
	g_wlanLedStatus = val;

	return count;
}
#endif

#ifdef SUPPORT_HARDWARE_MULTI_MODE
/* added by ZCF, 20110420 */
static void init_sysMode()
{
	if (ar7240_gpio_in_val(SYS_MODE_GPIO_1) 
		&& ar7240_gpio_in_val(SYS_MODE_GPIO_2))	// AP
	{
	#ifdef NORMAL_ROUTER_WITH_AP
		#ifdef MODE_APC_ROUTER
		g_sysMode = SYSTEM_MODE_APC_ROUTER;
		#endif
	#else
		g_sysMode = SYSTEM_MODE_AP;
	#endif
	}
	else if (!ar7240_gpio_in_val(SYS_MODE_GPIO_1)
		&& ar7240_gpio_in_val(SYS_MODE_GPIO_2))	// APC
	{
	#ifdef NORMAL_ROUTER_WITH_AP
		#ifdef MODE_NORMAL_ROUTER
		g_sysMode = SYSTEM_MODE_NORMAL_ROUTER;
		#endif
	#else
		#ifdef MODE_APC_ROUTER
		g_sysMode = SYSTEM_MODE_APC_ROUTER;
		#endif
		#ifdef MODE_NORMAL_ROUTER
		g_sysMode = SYSTEM_MODE_NORMAL_ROUTER;
		#endif
	#endif
	}
	else											// 3G
	{
		g_sysMode = SYSTEM_MODE_3G_ROUTER;
        
	}
}

void check_sysMode(unsigned long nothing)
{
	int system_mode;

	if (ar7240_gpio_in_val(SYS_MODE_GPIO_1) 
		&& ar7240_gpio_in_val(SYS_MODE_GPIO_2))	// AP
	{
	#ifdef NORMAL_ROUTER_WITH_AP
		#ifdef MODE_APC_ROUTER
		system_mode = SYSTEM_MODE_APC_ROUTER;
		#endif
	#else
		system_mode = SYSTEM_MODE_AP;
	#endif
	}
	else if (!ar7240_gpio_in_val(SYS_MODE_GPIO_1)
		&& ar7240_gpio_in_val(SYS_MODE_GPIO_2))	// APC
	{
	#ifdef NORMAL_ROUTER_WITH_AP
		#ifdef MODE_NORMAL_ROUTER
		system_mode = SYSTEM_MODE_NORMAL_ROUTER;
		#endif
	#else
		#ifdef MODE_APC_ROUTER
		system_mode = SYSTEM_MODE_APC_ROUTER;
		#endif
		#ifdef MODE_NORMAL_ROUTER
		system_mode = SYSTEM_MODE_NORMAL_ROUTER;
		#endif
	#endif
	}
	else											// 3G
	{
		system_mode = SYSTEM_MODE_3G_ROUTER;
	}
	if (g_sysMode != system_mode && !g_isSoftSysModeEnable)
	{
		//reboot router
		printk("rebooting...\n");
		ar7240_reg_rmw_set(AR7240_RESET, AR7240_RESET_FULL_CHIP);
		return;
	}
	
		mod_timer(&sysMode_timer, jiffies + 1 * HZ);
	}
#endif

#ifdef GPIO_RSSI_LED_SUPPORT
static int rssi_led_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
    return 0;
}

static int rssi_led_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
    u_int32_t val;
    u_int32_t reg_val;
    int gpioval;
    int bitval;
    
    if (sscanf(buf, "%d", &val) != 1)
    {
        return -EINVAL;
    }

    if (val > 4 || val < 0)
    {
        return -EINVAL;
    }

    gpioval = ar7240_reg_rd(AR7240_GPIO_OUT);
    switch (val)
    {
        case 0:
            /*turn off all led*/
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI1_LED_BIT, RSSI1_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI2_LED_BIT, RSSI2_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI3_LED_BIT, RSSI3_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI4_LED_BIT, RSSI4_LED_OFF);
            break;     
        case 1:
            /*rssi1 on*/
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI1_LED_BIT, RSSI1_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI2_LED_BIT, RSSI2_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI3_LED_BIT, RSSI3_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI4_LED_BIT, RSSI4_LED_OFF);
            break;
        case 2:
            /*rssi1&2 on*/
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI1_LED_BIT, RSSI1_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI2_LED_BIT, RSSI2_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI3_LED_BIT, RSSI3_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI4_LED_BIT, RSSI4_LED_OFF);
            break;
        case 3:
            /*rssi1&2&3 on*/
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI1_LED_BIT, RSSI1_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI2_LED_BIT, RSSI2_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI3_LED_BIT, RSSI3_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI4_LED_BIT, RSSI4_LED_OFF);
            break;
        case 4:
            /*rssi1&2&3&4 on*/
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI1_LED_BIT, RSSI1_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI2_LED_BIT, RSSI2_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI3_LED_BIT, RSSI3_LED_ON);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI4_LED_BIT, RSSI4_LED_ON); 
            break;
        default:
            /*turn off all led*/
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI1_LED_BIT, RSSI1_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI2_LED_BIT, RSSI2_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI3_LED_BIT, RSSI3_LED_OFF);
            AR7240_SET_BIT_VAL(gpioval, bitval, RSSI4_LED_BIT, RSSI4_LED_OFF);
            break;
    }

    ar7240_reg_wr(AR7240_GPIO_OUT, gpioval);
    return count;
}
#endif

static int gpio_system_mode_read (char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
	int system_mode;

	system_mode = g_sysMode;

	return sprintf(page, "%d\n", system_mode);
}
static int gpio_system_mode_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
	u_int32_t val = 0;
	
	if (sscanf(buf, "%d", &val) != 1)
        return -EINVAL;

	if (val < 0) 
		return -EINVAL;
	if (val == SYSTEM_MODE_SOFT_ENABLE)	/* disable reading system mode from gpio pins */
	{
		g_isSoftSysModeEnable = 1;
	}
	else if (val == SYSTEM_MODE_SOFT_DISABLE)	/* enable reading system mode from gpio pins */
	{
		g_isSoftSysModeEnable = 0;
	}
	else if (g_isSoftSysModeEnable)
	{
		g_sysMode = val;	/* set system mode from web server */
	}

	return count;
	}

#ifdef GPIO_JUMPSTART_LED_BIT
static int gpio_tricolor_led_read (char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    return sprintf (page, "%d\n", gpio_tricolorled);
}

static int gpio_tricolor_led_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
    u_int32_t val, green_led_onoff = 0, yellow_led_onoff = 0;

    if (sscanf(buf, "%d", &val) != 1)
        return -EINVAL;

//	printk("%s %d\n", __FUNCTION__,__LINE__);

    if (val >= LED_STATE_MAX)
        return -EINVAL;

    if (val == gpio_tricolorled)
    return count;

    switch (val) {
        case LED_STATE_OFF :
                green_led_onoff = OFF;   /* both LEDs OFF */
                yellow_led_onoff = OFF;
                break;

        case LED_STATE_GREEN:
                green_led_onoff = ON;    /* green ON, Yellow OFF */
                yellow_led_onoff = OFF;
                break;

        case LED_STATE_YELLOW:
                green_led_onoff = OFF;   /* green OFF, Yellow ON */
                yellow_led_onoff = ON;
                break;

        case LED_STATE_ORANGE:
                green_led_onoff = ON;    /* both LEDs ON */
                yellow_led_onoff = ON;
                break;
	}

//	printk("green_led_onoff = %d\n", green_led_onoff);
    ar7240_gpio_out_val (TRICOLOR_LED_GREEN_PIN, green_led_onoff);
    //ar7240_gpio_out_val (TRICOLOR_LED_YELLOW_PIN, yellow_led_onoff);
    gpio_tricolorled = val;

    return count;
}
#endif

static int create_simple_config_led_proc_entry (void)
{
    if (simple_config_entry != NULL) {
        printk ("Already have a proc entry for /proc/simple_config!\n");
        return -ENOENT;
    }

    simple_config_entry = proc_mkdir("simple_config", NULL);
    if (!simple_config_entry)
        return -ENOENT;

    simulate_push_button_entry = create_proc_entry ("push_button", 0644,
                                                      simple_config_entry);
    if (!simulate_push_button_entry)
        return -ENOENT;

    simulate_push_button_entry->write_proc = push_button_write;
    simulate_push_button_entry->read_proc = push_button_read;
	
#ifdef GPIO_JUMPSTART_LED_BIT
    tricolor_led_entry = create_proc_entry ("tricolor_led", 0644,
                                            simple_config_entry);
    if (!tricolor_led_entry)
        return -ENOENT;

    tricolor_led_entry->write_proc = gpio_tricolor_led_write;
    tricolor_led_entry->read_proc = gpio_tricolor_led_read;
#endif

#ifdef GPIO_INTERNET_LED_BIT
	/* for internet led blink */
	internet_led_status_entry = create_proc_entry ("internet_blink", 0666,
                                                      simple_config_entry);
	if (!internet_led_status_entry)
		return -ENOENT;
	
    internet_led_status_entry->write_proc = internet_led_status_write;
    internet_led_status_entry->read_proc = internet_led_status_read;
#endif

#ifdef GPIO_USB_POWER_SUPPORT
	/*added by ZQQ, 10.06.02 for usb power*/
	usb_power_entry = create_proc_entry("usb_power", 0666, simple_config_entry);
	if(!usb_power_entry)
		return -ENOENT;

	usb_power_entry->write_proc = usb_power_write;
	usb_power_entry->read_proc = usb_power_read;	
	/*end added*/
#endif

/*
 * by dyf, on 27Sept2012
 * WISP led should be related to WISP config, rather not certain specific PID.
 */
#ifdef MODE_APC_ROUTER
	wlan_led_entry = create_proc_entry("wlan_led", 0666, simple_config_entry);
	if (!wlan_led_entry)
		return -ENOENT;
	
	wlan_led_entry->read_proc = gpio_wlan_status_read;
	wlan_led_entry->write_proc = gpio_wlan_status_write;
#endif
    
	system_mode_entry = create_proc_entry("system_mode", 0666,
								simple_config_entry);
	if (!system_mode_entry)
		return -ENOENT;

	system_mode_entry->read_proc = gpio_system_mode_read;
	system_mode_entry->write_proc = gpio_system_mode_write;

#ifdef SUPPORT_HARDWARE_MULTI_MODE
	/* configure gpio as inputs */
	ar7240_gpio_config_input(SYS_MODE_GPIO_1);
	ar7240_gpio_config_input(SYS_MODE_GPIO_2);
#endif

#ifdef GPIO_RSSI_LED_SUPPORT
    /* added by zb */
    rssi_led_entry = create_proc_entry("rssi_led", 0644, simple_config_entry);
    if (!rssi_led_entry)
    {
        return -ENOENT;
    }
    rssi_led_entry->write_proc = rssi_led_write;
    rssi_led_entry->read_proc = rssi_led_read;
    /* end added */
#endif

    return 0;
}



/******* begin ioctl stuff **********/
#ifdef CONFIG_GPIO_DEBUG
void print_gpio_regs(char* prefix)
{
	printk("\n-------------------------%s---------------------------\n", prefix);
	printk("AR7240_GPIO_OE:%#X\n", ar7240_reg_rd(AR7240_GPIO_OE));
	printk("AR7240_GPIO_IN:%#X\n", ar7240_reg_rd(AR7240_GPIO_IN));
	printk("AR7240_GPIO_OUT:%#X\n", ar7240_reg_rd(AR7240_GPIO_OUT));
	printk("AR7240_GPIO_SET:%#X\n", ar7240_reg_rd(AR7240_GPIO_SET));
	printk("AR7240_GPIO_CLEAR:%#X\n", ar7240_reg_rd(AR7240_GPIO_CLEAR));
	printk("AR7240_GPIO_INT_ENABLE:%#X\n", ar7240_reg_rd(AR7240_GPIO_INT_ENABLE));
	printk("AR7240_GPIO_INT_TYPE:%#X\n", ar7240_reg_rd(AR7240_GPIO_INT_TYPE));
	printk("AR7240_GPIO_INT_POLARITY:%#X\n", ar7240_reg_rd(AR7240_GPIO_INT_POLARITY));
	printk("AR7240_GPIO_INT_PENDING:%#X\n", ar7240_reg_rd(AR7240_GPIO_INT_PENDING));
	printk("AR7240_GPIO_INT_MASK:%#X\n", ar7240_reg_rd(AR7240_GPIO_INT_MASK));
	printk("\n-------------------------------------------------------\n");
	}
#endif

/* ioctl for reset default detection and system led switch*/
int ar7240_gpio_ioctl(struct inode *inode, struct file *file,  unsigned int cmd, unsigned long arg)
{
	int i;
	int* argp = (int *)arg;

	if (_IOC_TYPE(cmd) != AR7240_GPIO_MAGIC ||
		_IOC_NR(cmd) < AR7240_GPIO_IOCTL_BASE ||
		_IOC_NR(cmd) > AR7240_GPIO_IOCTL_MAX)
	{
		printk("type:%d nr:%d\n", _IOC_TYPE(cmd), _IOC_NR(cmd));
		printk("ar7240_gpio_ioctl:unknown command\n");
		return -1;
	}

	switch (cmd)
	{
	case AR7240_GPIO_BTN_READ:
		*argp = counter;
		counter = 0;
		break;
			
	case AR7240_GPIO_LED_READ:
		printk("\n\n");
		for (i = 0; i < AR7240_GPIO_COUNT; i ++)
		{
			printk("pin%d: %d\n", i, ar7240_gpio_in_val(i));
		}
		printk("\n");

	#ifdef CONFIG_GPIO_DEBUG
		print_gpio_regs("");
	#endif
			
		break;
			
	case AR7240_GPIO_LED_WRITE:
		if (unlikely(bBlockWps))
			bBlockWps = 0;
#ifdef SYS_LED_GPIO
		ar7240_gpio_out_val(SYS_LED_GPIO, *argp);	
#endif
		break;
#ifdef AP_USB_LED_GPIO
	case AR7240_GPIO_USB_LED1_WRITE:
			ar7240_gpio_out_val(AP_USB_LED_GPIO, *argp);
			break;
#endif
    case AR7240_GPIO_WLANBTN_READ:
        *argp = wlan_counter;
		wlan_counter = 0;
        break;
	default:
		printk("command not supported\n");
		return -1;
	}


	return 0;
}


int ar7240_gpio_open (struct inode *inode, struct file *filp)
{
	int minor = iminor(inode);
	int devnum = minor; //>> 1;
	struct mtd_info *mtd;

	if ((filp->f_mode & 2) && (minor & 1))
{
		printk("You can't open the RO devices RW!\n");
		return -EACCES;
}

	mtd = get_mtd_device(NULL, devnum);   
	if (!mtd)
{
		printk("Can not open mtd!\n");
		return -ENODEV;	
	}
	filp->private_data = mtd;
	return 0;

}

/* struct for cdev */
struct file_operations gpio_device_op =
{
	.owner = THIS_MODULE,
	.ioctl = ar7240_gpio_ioctl,
	.open = ar7240_gpio_open,
};

/* struct for ioctl */
static struct cdev gpio_device_cdev =
{
	.owner  = THIS_MODULE,
	.ops	= &gpio_device_op,
};
/******* end  ioctl stuff **********/

int __init ar7240_simple_config_init(void)
{
    int req;
    int gpioval;
    int gpiooe;
    int bitval;

	/* restore factory default and system led */
	dev_t dev;
    int rt;
    int ar7240_gpio_major = gpio_major;
    int ar7240_gpio_minor = gpio_minor;

	init_timer(&rst_timer);
	rst_timer.function = check_rst;
    #ifdef WLAN_BTN_BIT
    init_timer(&wlan_timer);
    wlan_timer.function = check_wlan;
	#endif 
     
#ifdef CONFIG_PID_MR302001
	printk("\n\nWhoops! This kernel is for product mr3020 v1.0!\n\n");
#endif
#ifdef CONFIG_PID_WR74104
	printk("\n\nWhoops! This kernel is for product wr741 v4.0!\n\n");
#endif
#ifdef CONFIG_PID_MR322001
	printk("\n\nWhoops! This kernel is for product mr3220 v1.0!\n\n");
#endif
#ifdef CONFIG_PID_MR322002
	printk("\n\nWhoops! This kernel is for product mr3220 v2.0!\n\n");
#endif

#ifdef CONFIG_PID_WR70301
	printk("\n\nWhoops! This kernel is for product wr703 v1.0!\n\n");
#endif

#ifdef SUPPORT_HARDWARE_MULTI_MODE
	/* added by ZCF, 20110420 */
	ar7240_gpio_config_input(SYS_MODE_GPIO_1);
	ar7240_gpio_config_input(SYS_MODE_GPIO_2);
	init_sysMode();
	init_timer(&sysMode_timer);
	sysMode_timer.function = check_sysMode;
	mod_timer(&sysMode_timer, jiffies + 1 * HZ);
#endif

#ifndef JUMPSTART_RST_MULTIPLEXED
#ifdef JUMPSTART_GPIO
	init_timer(&wps_timer);
	wps_timer.function = check_wps;
	/* This is NECESSARY, lsz 090109 */
	ar7240_gpio_config_input(JUMPSTART_GPIO);
	 /* configure JUMPSTART_GPIO as level triggered interrupt */
    	ar7240_gpio_config_int (JUMPSTART_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_HIGH);

	 req = request_irq (AR7240_GPIO_IRQn(JUMPSTART_GPIO), jumpstart_irq, 0,
                       "SW_JUMPSTART", NULL);
    if (req != 0)
	{
        printk (KERN_ERR "unable to request IRQ for SWJUMPSTART GPIO (error %d)\n", req);
    }
#endif
#endif

#ifdef WLAN_BTN_BIT
    ar7240_gpio_config_input(WLAN_BTN_BIT);
    ar7240_gpio_config_int (WLAN_BTN_BIT, INT_TYPE_LEVEL, INT_POL_ACTIVE_HIGH);

    req = request_irq (AR7240_GPIO_IRQn(WLAN_BTN_BIT), wlanbit_irq, 0,
                   "SW_WLANBIT", NULL);
    if (req != 0)
    {
    }

#endif


#ifdef GPIO_INTERNET_LED_BIT
	g_internetLedPin = GPIO_INTERNET_LED_BIT;
#endif
#ifdef AP_USB_LED_GPIO
	ar7240_gpio_config_output(AP_USB_LED_GPIO);
	/* init Internet LED status (off) */
	ar7240_gpio_out_val(AP_USB_LED_GPIO, USB_LED_OFF);
#endif

#ifdef GPIO_WLAN_LED_BIT
	/* init WLAN LED status (off) */
	ar7240_gpio_out_val(WLAN_LED_GPIO, WLAN_LED_OFF);
#endif

#ifdef SYS_LED_GPIO
	/* configure SYS_LED_GPIO as output led */
	ar7240_gpio_config_output(SYS_LED_GPIO);
	ar7240_gpio_out_val(SYS_LED_GPIO, SYS_LED_ON);
#endif

#ifdef GPIO_JUMPSTART_LED_BIT
	/* configure gpio as outputs */
    ar7240_gpio_config_output (TRICOLOR_LED_GREEN_PIN); 
    /* switch off the led */
    ar7240_gpio_out_val(TRICOLOR_LED_GREEN_PIN, OFF);
#endif

#ifdef GPIO_USB_POWER_SUPPORT
	/* configure gpio as outputs */
    ar7240_gpio_config_output (SYS_USB_POWER_GPIO); 
    /* power on usb modem */
    ar7240_gpio_out_val(SYS_USB_POWER_GPIO, USB_POWER_ON);
#endif

#ifdef GPIO_RSSI_LED_SUPPORT
    /* configure gpio as outputs, used for rssi led control*/
    gpiooe = ar7240_reg_rd(AR7240_GPIO_OE);
    AR7240_SET_BIT_VAL(gpiooe, bitval, RSSI1_LED_BIT, 1);
    AR7240_SET_BIT_VAL(gpiooe, bitval, RSSI2_LED_BIT, 1);
    AR7240_SET_BIT_VAL(gpiooe, bitval, RSSI3_LED_BIT, 1);
    AR7240_SET_BIT_VAL(gpiooe, bitval, RSSI4_LED_BIT, 1);
    
    /*default led state, all on*/
    gpioval = ar7240_reg_rd(AR7240_GPIO_OUT);
    AR7240_SET_BIT_VAL(gpioval, bitval, RSSI1_LED_BIT, RSSI1_LED_ON);
    AR7240_SET_BIT_VAL(gpioval, bitval, RSSI2_LED_BIT, RSSI2_LED_ON);
    AR7240_SET_BIT_VAL(gpioval, bitval, RSSI3_LED_BIT, RSSI3_LED_ON);
    AR7240_SET_BIT_VAL(gpioval, bitval, RSSI4_LED_BIT, RSSI4_LED_ON);

    /*write the reg*/
    ar7240_reg_wr(AR7240_GPIO_OE, gpiooe);
    ar7240_reg_wr(AR7240_GPIO_OUT, gpioval);
#endif

    create_simple_config_led_proc_entry ();

	ar7240_gpio_config_input(RST_DFT_GPIO);

	/* configure GPIO RST_DFT_GPIO as level triggered interrupt */
	ar7240_gpio_config_int (RST_DFT_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_HIGH);

    rt = request_irq (AR7240_GPIO_IRQn(RST_DFT_GPIO), rst_irq, 0,
                       "RESTORE_FACTORY_DEFAULT", NULL);
    if (rt != 0)
	{
        printk (KERN_ERR "unable to request IRQ for RESTORE_FACTORY_DEFAULT GPIO (error %d)\n", rt);
    }

    if (ar7240_gpio_major)
	{
        dev = MKDEV(ar7240_gpio_major, ar7240_gpio_minor);
        rt = register_chrdev_region(dev, 1, "ar7240_gpio_chrdev");
    }
	else
	{
        rt = alloc_chrdev_region(&dev, ar7240_gpio_minor, 1, "ar7240_gpio_chrdev");
        ar7240_gpio_major = MAJOR(dev);
    }

    if (rt < 0)
	{
        printk(KERN_WARNING "ar7240_gpio_chrdev : can`t get major %d\n", ar7240_gpio_major);
        return rt;
    }

    cdev_init (&gpio_device_cdev, &gpio_device_op);
    rt = cdev_add(&gpio_device_cdev, dev, 1);
	
    if (rt < 0) 
		printk(KERN_NOTICE "Error %d adding ar7240_gpio_chrdev ", rt);

    return 0;
}

subsys_initcall(ar7240_simple_config_init);

