/*! Copyright(c) 2008-2009 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file		wlanWarnTarget.h
 *\brief	header file of wlan warn kernel module
 *
 *\author	Zou Boyin
 *\version	1.0.0
 *\date		27Dec09
 *
 *\history  \arg 1.0.0, 27Dec09, Zou Boyin, Create the file.
 */
 
#ifndef __WLAN_WARN_KERNEL_H__
#define __WLAN_WARN_KERNEL_H__

/***************************************************************************/
/*						 CONFIGURATIONS							 		   */
/***************************************************************************/
#define WLAN_WARN_TARGET_DEBUG 1

/***************************************************************************/
/*						DEFINES						 					   */
/***************************************************************************/
/* start */
/* the command number for getsocket and setsockopt */
#define WLAN_WARN_SO_BASE			96 
#define WLAN_WARN_SO_SET_PARM  		(WLAN_WARN_SO_BASE)
#define WLAN_WARN_SO_SET_MIN		WLAN_WARN_SO_SET_PARM
#define WLAN_WARN_SO_SET_MAX		(WLAN_WARN_SO_SET_PARM + 1)
#define WLAN_WARN_SO_GET_PARM		(WLAN_WARN_SO_BASE)
#define WLAN_WARN_SO_GET_MIN		WLAN_WARN_SO_GET_PARM
#define WLAN_WARN_SO_GET_MAX		(WLAN_WARN_SO_GET_PARM + 1)
/* end */

/*!
*\def		CONFIG_PATTERN
*\brief		the configure information pattern in url
*/
#define CONFIG_PATTERN		"TPWlanWarnChecked="

#define CONFIG_PATTERN_YES	'1'
#define CONFIG_PATTERH_NO	'0'


#define CMP_NEED_PARM		(1)
#define CMP_NONE			(2)

#undef PDEBUG
#if WLAN_WARN_TARGET_DEBUG
	#ifdef __KERNEL__
		#define PDEBUG(fmt, args...) printk(KERN_ALERT   fmt, ##args)
	#else
		#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
	#endif
#else
	#define PDEBUG(fmt, args...) 
#endif /* #if WLAN_WARN_TARGET_DEBUG */



/*!
*\enum		CHECK_STATE
*\brief 	the check state of warning page
*/
enum CHECK_STATE
{
	CHECK_NONE = 0,	/*!< url doesn't contains check info 	*/
	CHECKED, 		/*!< the option is checked by user 		*/
	NOT_CHECKED		/*!< the option is no checked by user 	*/
 }; 
 


/***************************************************************************/
/*								TYPES									   */
/***************************************************************************/
/*!
 *\struct 	ipt_wlan_warn_info
 *\brief 	use to communicate between iptables target and iptables user space program
			NOT USED in this moment
 */
struct ipt_wlan_warn_info {
	char empty;
};

/*!
 *\struct 	WLAN_WARN_PARAM
 *\brief 	use to communicate between kernel and user space
 */
typedef struct _WLAN_WARN_PARAM
{
	struct iphdr 	ipHeader;	/*!< the ip header of user HTTP GET packet  */
	struct tcphdr 	tcpHeader;	/*!< the tcp header of user HTTP GET packet */
	char			checked;	/*!< does user check "Not Display Any More" */
}WLAN_WARN_PARAM;

#endif /* #ifndef __WLAN_WARN_KERNEL_H__ */
