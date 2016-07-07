//added by hlf 

#ifndef __CDC_NCM_H
#define __CDC_NCM_H

//cdc.h
#define USB_CDC_SUBCLASS_NCM			0x0d
#define USB_CDC_NCM_PROTO_NTB			1
#define USB_CDC_NCM_TYPE		0x1a


/* "NCM Control Model Functional Descriptor" */
struct usb_cdc_ncm_desc {
	__u8	bLength;
	__u8	bDescriptorType;
	__u8	bDescriptorSubType;

	__le16	bcdNcmVersion;
	__u8	bmNetworkCapabilities;
} __attribute__ ((packed));


#define USB_CDC_GET_NTB_PARAMETERS		0x80
#define USB_CDC_GET_NTB_FORMAT			0x83
#define USB_CDC_SET_NTB_FORMAT			0x84
#define USB_CDC_SET_NTB_INPUT_SIZE		0x86
#define USB_CDC_SET_CRC_MODE			0x8a
#define USB_CDC_GET_MAX_DATAGRAM_SIZE		0x87
#define USB_CDC_SET_MAX_DATAGRAM_SIZE		0x88

struct usb_cdc_speed_change {
	__le32	DLBitRRate;	/* contains the downlink bit rate (IN pipe) */
	__le32	ULBitRate;	/* contains the uplink bit rate (OUT pipe) */
} __attribute__ ((packed));
/*
 * Class Specific structures and constants
 *
 * CDC NCM NTB parameters structure, CDC NCM subclass 6.2.1
 *
 */

struct usb_cdc_ncm_ntb_parameters {
	__le16	wLength;
	__le16	bmNtbFormatsSupported;
	__le32	dwNtbInMaxSize;
	__le16	wNdpInDivisor;
	__le16	wNdpInPayloadRemainder;
	__le16	wNdpInAlignment;
	__le16	wPadding1;
	__le32	dwNtbOutMaxSize;
	__le16	wNdpOutDivisor;
	__le16	wNdpOutPayloadRemainder;
	__le16	wNdpOutAlignment;
	__le16	wNtbOutMaxDatagrams;
} __attribute__ ((packed));

/*
 * CDC NCM transfer headers, CDC NCM subclass 3.2
 */

#define USB_CDC_NCM_NTH16_SIGN		0x484D434E /* NCMH */
#define USB_CDC_NCM_NTH32_SIGN		0x686D636E /* ncmh */

struct usb_cdc_ncm_nth16 {
	__le32	dwSignature;
	__le16	wHeaderLength;
	__le16	wSequence;
	__le16	wBlockLength;
	__le16	wNdpIndex;
} __attribute__ ((packed));

struct usb_cdc_ncm_nth32 {
	__le32	dwSignature;
	__le16	wHeaderLength;
	__le16	wSequence;
	__le32	dwBlockLength;
	__le32	dwNdpIndex;
} __attribute__ ((packed));

/*
 * CDC NCM datagram pointers, CDC NCM subclass 3.3
 */

#define USB_CDC_NCM_NDP16_CRC_SIGN	0x314D434E /* NCM1 */
#define USB_CDC_NCM_NDP16_NOCRC_SIGN	0x304D434E /* NCM0 */
#define USB_CDC_NCM_NDP32_CRC_SIGN	0x316D636E /* ncm1 */
#define USB_CDC_NCM_NDP32_NOCRC_SIGN	0x306D636E /* ncm0 */

/* 16-bit NCM Datagram Pointer Entry */
struct usb_cdc_ncm_dpe16 {
	__le16	wDatagramIndex;
	__le16	wDatagramLength;
} __attribute__((__packed__));

/* 16-bit NCM Datagram Pointer Table */
struct usb_cdc_ncm_ndp16 {
	__le32	dwSignature;
	__le16	wLength;
	__le16	wNextNdpIndex;
	struct	usb_cdc_ncm_dpe16 dpe16[0];
} __attribute__ ((packed));

/* 32-bit NCM Datagram Pointer Entry */
struct usb_cdc_ncm_dpe32 {
	__le32	dwDatagramIndex;
	__le32	dwDatagramLength;
} __attribute__((__packed__));

/* 32-bit NCM Datagram Pointer Table */
struct usb_cdc_ncm_ndp32 {
	__le32	dwSignature;
	__le16	wLength;
	__le16	wReserved6;
	__le32	dwNextNdpIndex;
	__le32	dwReserved12;
	struct	usb_cdc_ncm_dpe32 dpe32[0];
} __attribute__ ((packed));

/* CDC NCM subclass 3.2.1 and 3.2.2 */
#define USB_CDC_NCM_NDP16_INDEX_MIN			0x000C
#define USB_CDC_NCM_NDP32_INDEX_MIN			0x0010

/* CDC NCM subclass 3.3.3 Datagram Formatting */
#define USB_CDC_NCM_DATAGRAM_FORMAT_CRC			0x30
#define USB_CDC_NCM_DATAGRAM_FORMAT_NOCRC		0X31

/* CDC NCM subclass 4.2 NCM Communications Interface Protocol Code */
#define USB_CDC_NCM_PROTO_CODE_NO_ENCAP_COMMANDS	0x00
#define USB_CDC_NCM_PROTO_CODE_EXTERN_PROTO		0xFE

/* CDC NCM subclass 5.2.1 NCM Functional Descriptor, bmNetworkCapabilities */
#define USB_CDC_NCM_NCAP_ETH_FILTER			(1 << 0)
#define USB_CDC_NCM_NCAP_NET_ADDRESS			(1 << 1)
#define USB_CDC_NCM_NCAP_ENCAP_COMMAND			(1 << 2)
#define USB_CDC_NCM_NCAP_MAX_DATAGRAM_SIZE		(1 << 3)
#define USB_CDC_NCM_NCAP_CRC_MODE			(1 << 4)
#define	USB_CDC_NCM_NCAP_NTB_INPUT_SIZE			(1 << 5)

/* CDC NCM subclass Table 6-3: NTB Parameter Structure */
#define USB_CDC_NCM_NTB16_SUPPORTED			(1 << 0)
#define USB_CDC_NCM_NTB32_SUPPORTED			(1 << 1)

/* CDC NCM subclass Table 6-3: NTB Parameter Structure */
#define USB_CDC_NCM_NDP_ALIGN_MIN_SIZE			0x04
#define USB_CDC_NCM_NTB_MAX_LENGTH			0x1C

/* CDC NCM subclass 6.2.5 SetNtbFormat */
#define USB_CDC_NCM_NTB16_FORMAT			0x00
#define USB_CDC_NCM_NTB32_FORMAT			0x01

/* CDC NCM subclass 6.2.7 SetNtbInputSize */
#define USB_CDC_NCM_NTB_MIN_IN_SIZE			2048
#define USB_CDC_NCM_NTB_MIN_OUT_SIZE			2048

/* NTB Input Size Structure */
struct usb_cdc_ncm_ndp_input_size {
	__le32	dwNtbInMaxSize;
	__le16	wNtbInMaxDatagrams;
	__le16	wReserved;
} __attribute__ ((packed));

/* CDC NCM subclass 6.2.11 SetCrcMode */
#define USB_CDC_NCM_CRC_NOT_APPENDED			0x00
#define USB_CDC_NCM_CRC_APPENDED			0x01

#endif /* __CDC_NCM_H */
