
/**************************************************************************************
* File Name  : tpLinuxTag.h
*
* Description: add tag with validation system to the firmware image file to be uploaded
*              via http
*
* Created    : 16Sep07,	Liang Qiming
**************************************************************************************/

#ifndef _TP_LINUX_TAG_H_
#define _TP_LINUX_TAG_H_


#define TP_SIG_1   "TP-LINK Technologies"
#define TP_SIG_2   "ver. 1.0"          // was "firmware version 2.0" now it is split 6 char out for chip id.

#define TP_TAG_VER         "6"
#define TP_TAG_VER_LAST    "26"

// file tag (head) structure all is in clear text except validationTokens (crc, md5, sha1, etc). Total: 128 unsigned chars
#define TAG_LEN         	512
#define SIG_LEN         	24
#define SIG_LEN_2       	12   // Original second SIG = 20 is now devided into 14 for SIG_LEN_2 and 6 for CHIP_ID
#define CHIP_ID_LEN			8	
#define TOKEN_LEN       	20
#define BOARD_ID_LEN   		16


// TAG for downloadable image (kernel plus file system)
// integer in the structure is stored in Network-Byte order (BIG-endian)
typedef struct _LINUX_FILE_TAG
{
	unsigned long tagVersion;
    char 		  signiture_1[SIG_LEN];         // text line for company info
	char 		  signiture_2[SIG_LEN_2];       // additional info (can be version number)
    char 		  chipId[CHIP_ID_LEN];			// chip id 
    char 		  boardId[BOARD_ID_LEN];        // board id
    unsigned long productId;					// product id
    unsigned long productVer;					// product version
    unsigned long reserved1;					// reserved for future

    unsigned char imageValidationToken[TOKEN_LEN];	// image validation token - md5 checksum
    unsigned char kernelValidationToken[TOKEN_LEN];	// kernel+tag validation token - md5 checksum

	unsigned long kernelTextAddr;				// text section address of kernel
	unsigned long kernelEntryPoint;				// entry point address of kernel
	
	unsigned long totalImageLen;				// the sum of kernelLen+rootfsLen+tagLen

	unsigned long kernelAddress;				// starting address (offset from the beginning of FILE_TAG) of kernel image
	unsigned long kernelLen;					// length of kernel image

	unsigned long rootfsAddress;				// starting address (offset) of filesystem image
	unsigned long rootfsLen;					// length of filesystem image

	unsigned long bootloaderAddress;			// starting address (offset) of boot loader image
	unsigned long bootloaderLen;				// length of boot loader image
	
} LINUX_FILE_TAG;

typedef unsigned int	Elf32_Addr;

#define FILE_TAG_SIZE			512
#define LINUX_KERNEL_OFFSET		(FILE_TAG_SIZE)

typedef struct _LINUX_FLASH_STRUCT
{
	unsigned long bootOffset;
	unsigned long macAddrOffset;
	unsigned long pinOffset;
	unsigned long kernelOffset;
	unsigned long rootfsOffset;
	unsigned long configOffset;
	unsigned long radioOffset;
} LINUX_FLASH_STRUCT;

unsigned long tp_htonl(unsigned long value);
unsigned short tp_htons(unsigned short value);

int getElfInfo(char *elfFile, Elf32_Addr *eEntry, Elf32_Addr *pVaddr);

#endif // _TP_LINUX_TAG_H_

