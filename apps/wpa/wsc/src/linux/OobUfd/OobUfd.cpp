/*============================================================================
//
// Copyright(c) 2006 Intel Corporation. All rights reserved.
//   All rights reserved.
// 
//   Redistribution and use in source and binary forms, with or without 
//   modification, are permitted provided that the following conditions 
//   are met:
// 
//     * Redistributions of source code must retain the above copyright 
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright 
//       notice, this list of conditions and the following disclaimer in 
//       the documentation and/or other materials provided with the 
//       distribution.
//     * Neither the name of Intel Corporation nor the names of its 
//       contributors may be used to endorse or promote products derived 
//       from this software without specific prior written permission.
// 
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
//   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  File Name : OobUfd.cpp
//  Description: This file contains implementation of functions for the
//               USB OOB Media manager.
//  
//===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "tutrace.h"
#include "WscCommon.h"
#include "WscError.h"
#include "Portability.h"
#include "OobUfd.h"

#define TU_USB_ENROLLEE_PIN_FILE_NAME         "%02x%02x%02x%02x.WFA"
#define TU_USB_REGISTRAR_PIN_FILE_NAME        "00000000.WFA"
#define TU_USB_UNENCRYPTED_SETTINGS_FILE_NAME "00000000.WSC"
#define TU_USB_ENCRYPTED_SETTINGS_FILE_NAME   "%02x%02x%02x%02x.WSC"
#define TU_USB_DIR_NAME             "SMRTNTKY/WFAWSC"

#define TU_USB_RECV_LEN             2048
#define TU_SCSI_USB_PATH            "/proc/scsi/usb-storage"
#define TU_PROC_MOUNTS              "/proc/mounts"
#define TU_DEV_UFD                  "/dev/sd"
#define MAX_LINE_LENGTH             256

#define TU_USB_INSERTED     1
#define TU_USB_REMOVED      0

COobUfd::COobUfd()
{
    TUTRACE((TUTRACE_INFO, "COobUfd Construction\n"));
    m_monitorEvent = 0;
    m_monitorThreadHandle = 0;
}

COobUfd::~COobUfd()
{
    TUTRACE((TUTRACE_INFO, "COobUfd Destruction\n"));
}


uint32 COobUfd::Init()
{
    uint32 retVal;

    retVal = WscCreateEvent( &m_monitorEvent);
    if (retVal != WSC_SUCCESS)
    {
        TUTRACE((TUTRACE_ERR, "CreateEvent failed.\n"));
        return retVal;
    }

    m_initialized = true;

    return WSC_SUCCESS;
}

uint32 COobUfd::StartMonitor()
{
    uint32 retVal;

    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

	m_killRecvTh = false;
	m_killMoniTh = false;

    // create thread for monitoring
    retVal = WscCreateThread(&m_monitorThreadHandle, 
                                StaticMonitorThread,
                                this);
    if (retVal != WSC_SUCCESS)
    {
        TUTRACE((TUTRACE_ERR,  "Creating Monitor Thread failed.\n"));
        return retVal;
    }

    WscResetEvent(m_monitorEvent);
    return WSC_SUCCESS;
}

uint32 COobUfd::StopMonitor()
{
	TUTRACE((TUTRACE_INFO, "In StopMonitor\n"));

    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

	m_killRecvTh = true;
	m_killMoniTh = true;

	TUTRACE((TUTRACE_INFO, "bools are set to kill\n"));
    WscDestroyThread(m_monitorThreadHandle);
    m_monitorThreadHandle = 0;
    return WSC_SUCCESS;
}

uint32 COobUfd::GetUFDFileName( char * fileName, int maxFileName, EOobDataType oobdType, uint8 *macAddr )
{
	if (strlen(m_usbDir) == 0) { 
		return WSC_ERR_NOT_INITIALIZED;
	}

	sprintf(fileName, "%s/%s/", m_usbDir, TU_USB_DIR_NAME);

	char tmp[100];
	sprintf(tmp, "mkdir -p %s/%s", m_usbDir, TU_USB_DIR_NAME);
	system(tmp);

	tmp[0] = '\0';
	switch (oobdType)
	{
		case OOBD_TYPE_UNENCRYPTED: 
			strcpy(tmp, TU_USB_UNENCRYPTED_SETTINGS_FILE_NAME);
			break;

		case OOBD_TYPE_ENROLLEE_PIN:
			sprintf( tmp, TU_USB_ENROLLEE_PIN_FILE_NAME, 
				macAddr[0], macAddr[1],	macAddr[2], macAddr[3] );
			break;

		case OOBD_TYPE_REGISTRAR_PIN:
			strcpy(tmp, TU_USB_REGISTRAR_PIN_FILE_NAME);
			break;

		case OOBD_TYPE_ENCRYPTED:
			sprintf( tmp, TU_USB_ENCRYPTED_SETTINGS_FILE_NAME, 
				macAddr[0], macAddr[1],	macAddr[2], macAddr[3] );
			break;

		default:
			TUTRACE((TUTRACE_ERR, "Unknown OOBD_TYPE:%d\n", oobdType));
			return WSC_ERR_INVALID_PARAMETERS;
	}

	// If the type is Enrollee PIN but the MAC address is all zeros, then
	// search the directory for the first .WFA file based on an Enrollee MAC address.
	//
	if (oobdType == OOBD_TYPE_ENROLLEE_PIN && 
		strcmp(tmp, TU_USB_REGISTRAR_PIN_FILE_NAME) == 0) {

		struct dirent **namelist;
		int n;

		n = scandir(fileName, &namelist, 0, alphasort);
		bool foundone = false;
		if (n >= 0) { // found files
			unsigned int prefix;
			char ext[5];
			while(n--) {
				if ( (! foundone) &&
					 (sscanf(namelist[n]->d_name,"%8x.%s",&prefix,ext) == 2) &&
					 (prefix != 0) && 
					 (strcasecmp(ext,"WFA") == 0) ) {
						foundone = true;
						strcpy(tmp,namelist[n]->d_name); // keep first matching file name
					}
				free(namelist[n]);
			}
			free(namelist);
		}
	}
	strcat(fileName, tmp);
    return WSC_SUCCESS;
}

uint32 COobUfd::WriteOobData(EOobDataType oobdType, uint8 *p_mac, BufferObj &buff)
{
    // int hFile; 
	FILE * hFile;
    char fileName[MAX_FILEPATH_LENGTH];
    int32  nBytesWritten;
    char umtCmd[MAX_FILEPATH_LENGTH];
    int32 retVal;

	TUTRACE((TUTRACE_INFO, "In WriteOobData dataLen:%d\n", buff.Length()));

    if ( ! m_ufdFound)
    {
        TUTRACE((TUTRACE_ERR, "No USB Key inserted\n"));
        return TRUFD_ERR_DRIVE_REMOVED;
    }

	GetUFDFileName(fileName, MAX_FILEPATH_LENGTH, oobdType, p_mac);
    
	retVal = unlink(fileName);
	if (retVal == 0) {
		TUTRACE((TUTRACE_INFO, "Deleted file:[%s] on UFD\n", fileName));
	}
	/*  If cannot delete file, ignore and keep going.
    if (retVal == -1)
    {
        TUTRACE((TUTRACE_ERR, "Could not delete the file\n"));
        return TRUFD_ERR_FILEDELETE;
    }
	*/

    TUTRACE((TUTRACE_INFO, "Going to create file:[%s] on UFD\n", fileName));

    hFile = fopen(fileName, "w");
 
    if (hFile == NULL) 
    { 
        TUTRACE((TUTRACE_ERR, "File Creation Failed\n"));
        return TRUFD_ERR_FILEOPEN;
    }

    nBytesWritten = fwrite(buff.GetBuf(), 1, buff.Length(), hFile);

    if (nBytesWritten == 0)
    {
        TUTRACE((TUTRACE_ERR, "Could not write file\n"));
        fclose(hFile);
        return TRUFD_ERR_FILEWRITE;
    }

    if (nBytesWritten != (int32) buff.Length())
    {
        TUTRACE((TUTRACE_ERR, "Could not write all bytes\n"));
        fclose(hFile);
        return TRUFD_ERR_FILEWRITE;
    }

    fclose(hFile);

	WscSleep(1);
	sprintf(umtCmd, "umount %s", m_mountPath);
	system(umtCmd);
	TUTRACE((TUTRACE_INFO, "Unmounted USB Key from %s\n", m_mountPath));

    return WSC_SUCCESS;
}

uint32 COobUfd::WriteData(char * dataBuffer, uint32 dataLen)
{
    return WSC_ERR_NOT_IMPLEMENTED;
}

uint32 COobUfd::ReadData(char * dataBuffer, uint32 * dataLen)
{
    return WSC_ERR_NOT_IMPLEMENTED;
}

uint32 COobUfd::UfdDeleteFile(char * fileName)
{
    return WSC_ERR_NOT_IMPLEMENTED;
}

uint32 COobUfd::Deinit()
{
    TUTRACE((TUTRACE_INFO, "In UsbDeinit\n"));
    if ( ! m_initialized)
    {
        return WSC_ERR_NOT_INITIALIZED;
    }

    if (m_monitorThreadHandle)
    {
        WscDestroyThread(m_monitorThreadHandle);
        m_monitorThreadHandle = 0;
    }

    if (m_monitorEvent)
    {
        WscDestroyEvent(m_monitorEvent);
        m_monitorEvent = 0;
    }

    m_initialized = false;

    return WSC_SUCCESS;
}


void * COobUfd::StaticMonitorThread(IN void *p_data)
{
    TUTRACE((TUTRACE_INFO, "In StaticMonitorThread\n"));
    ((COobUfd *)p_data)->ActualMonitorThread();
    return 0;
} // StaticMonitorThread


void * COobUfd::ActualMonitorThread()
{
    char lineStr[MAX_LINE_LENGTH];

    // handles[0] = m_monitorEvent;

    TUTRACE((TUTRACE_INFO, "Usb MonitorThread Started\n"));

    m_ufdFound = false;

    TUTRACE((TUTRACE_INFO, "Going into while\n"));
    while (! m_killMoniTh)
    {
        while ( ! m_ufdFound && ! m_killMoniTh)
        {
            sleep(2);

            // check for existance of usb-storage
            if (access(TU_SCSI_USB_PATH, F_OK) != 0)
            {
                // TUTRACE((TUTRACE_INFO, "File Open Error\n"));
                continue;
            }

            // find the mount dir
            FILE * fp = fopen(TU_PROC_MOUNTS, "r");
            if (fp == NULL)
            {
                // TUTRACE((TUTRACE_INFO, "File Open Error\n"));
                continue;
            }

            while ( ( ! m_ufdFound) && fgets(lineStr, MAX_LINE_LENGTH, fp) != NULL)
            {
                // printf("read line:[%s]\n", lineStr);
                if (strstr(lineStr, TU_DEV_UFD) != NULL)
                {
                    // found the right line; get the second token
                    strcpy(m_mountPath, strtok(lineStr, " "));
                    strcpy(m_usbDir, strtok(NULL, " "));
                    TUTRACE((TUTRACE_INFO, "mountPath:[%s] usbDir:[%s]\n", 
					          m_mountPath, m_usbDir));

                    m_ufdFound = true;
                    // Send Notification
                    SendNotification(UFD_INSERTED);

                }
            } // inner while

            // close file
            fclose(fp);

        } // outer while

        // now start looking for usb drive removal
        while (m_ufdFound && ! m_killMoniTh)
        {
            sleep(2);

            // check for existance of usb-storage
            if (access(TU_SCSI_USB_PATH, F_OK) == 0)
            {
                // TUTRACE((TUTRACE_INFO, "File still exists\n"));
                continue;
            }
            else
            {
                TUTRACE((TUTRACE_ERR, "USB Flash Drive removed\n"));
                m_usbDir[0] = 0;
                m_ufdFound = false;
                // Send Notification
                SendNotification(UFD_REMOVED);
            }
        } // while

    } // outermost while(1)

    TUTRACE((TUTRACE_INFO, "Usb MonitorThread Exiting\n"));

    return 0;
}

void * COobUfd::StaticRecvThread(IN void *p_data)
{
    return 0;
} // StaticRecvThread


void * COobUfd::ActualRecvThread(void)
{
    return 0;
}

uint32 COobUfd::SendNotification(EUfdStatusCode status)
{
    S_CB_COMMON * ufdNotify;

    ufdNotify = new S_CB_COMMON;
    if (ufdNotify == NULL)
    {
        TUTRACE((TUTRACE_ERR, "Allocating for notify buffer failed\n"));
        return WSC_ERR_OUTOFMEMORY;
    }
    else
    {
        if (status == UFD_INSERTED)
        {
            ufdNotify->cbHeader.eType = CB_TRUFD_INSERTED;
        }
        else if (status == UFD_REMOVED)
        {
            ufdNotify->cbHeader.eType = CB_TRUFD_REMOVED;
        }
        else
        {
            TUTRACE((TUTRACE_ERR, "Invalid EUfdStatusCode\n"));
            return WSC_ERR_INVALID_PARAMETERS;
        }

        ufdNotify->cbHeader.dataLength = 0;

        m_trCallbackInfo.pf_callback(ufdNotify, m_trCallbackInfo.p_cookie);
    }
    return WSC_SUCCESS;
}

uint32 COobUfd::ReadOobData(EOobDataType oobdType, uint8 *p_mac, BufferObj &buff)
{
    uint32 retVal;
    char umtCmd[MAX_FILEPATH_LENGTH];
    FILE * hFile; 
    char fileName[MAX_FILEPATH_LENGTH];
    int32  bytesRead, nBytes = 0;
    struct stat flStat;
	int32 dataLen;
	uint8 dataBuffer[BUF_BLOCK_SIZE+10];

    if ( ! m_ufdFound)
    {
        TUTRACE((TUTRACE_ERR, "No USB Key inserted\n"));
        return TRUFD_ERR_DRIVE_REMOVED;
    }

	GetUFDFileName(fileName, MAX_FILEPATH_LENGTH, oobdType, p_mac);

	TUTRACE((TUTRACE_INFO, "Trying to read from file:%s\n", fileName));

    // first check size
    retVal = stat(fileName, &flStat); 
    if (retVal != 0)
    {
        TUTRACE((TUTRACE_ERR, "Getting file size failed\n"));
        return TRUFD_ERR_FILEOPEN;
    }

    dataLen = flStat.st_size;

    hFile = fopen(fileName, "r");
 
    if (hFile == NULL) 
    { 
        TUTRACE((TUTRACE_ERR, "File Open Failed\n"));
        return TRUFD_ERR_FILEOPEN;
    }

	while (BUF_BLOCK_SIZE == (bytesRead = fread(dataBuffer, 1, BUF_BLOCK_SIZE, hFile)))
	{
		buff.Append(bytesRead, dataBuffer);
		nBytes += bytesRead;
	}

	if (bytesRead > 0)
	{
		nBytes += bytesRead;
		buff.Append(bytesRead, dataBuffer);
	}

	if (!feof(hFile))
	{
		TUTRACE((TUTRACE_ERR, "Could not read the file\n"));
		fclose(hFile);
		return TRUFD_ERR_FILEREAD;
	}

    if (nBytes != dataLen)
    {
        TUTRACE((TUTRACE_ERR, "Could not read the entire file. Filesize=%d, Read only=%d\n", 
					dataLen, nBytes));
        fclose(hFile);
        return TRUFD_ERR_FILEREAD;
    }

    fclose(hFile);

	TUTRACE((TUTRACE_INFO, "Read from file:%s Successful\n", fileName));
	sprintf(umtCmd, "umount %s", m_mountPath);
	system(umtCmd);
	TUTRACE((TUTRACE_INFO, "Unmounted USB Key from %s\n", m_mountPath));
    return WSC_SUCCESS;
}

uint32 COobUfd::ReadDataAndSendItUp(void)
{
	return WSC_SUCCESS;
}

