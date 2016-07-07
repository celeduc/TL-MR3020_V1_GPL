/****************************************************************************
 *	File name:  msgid.h
 *	Author:     henry shao  (hshao@atheros.com)
 *      Date:       11/11/2006
 *	Copyright@  2006   Atheros communication
****************************************************************************/
#ifndef _MSG_ID_H
#define  _MSG_ID_H

#ifdef _MSG_TEXT_DUMP_
char* msgID[100]={ \
"","AP_CHAN","ASSOC_STATE","AUTH_TYPE","AUTH_TYPE_FLG","AUTHENTICATOR","","", \
"CONFIG_MTHD","CONFIG_ERR","CONF_URL4","CONF_URL6","CONN_TYPE","CONN_TYPE_FLG", \
"CREDENTIAL","ENCR_TYPE","ENCR_TYPE_FLG","DEV_NAME","DEV_PWDID","", \
"E_HASH1","E_HASH2","E_SNONCE1","E_SNONCE2","ENCR_SETTINGS","","ENROLLEE_NONCE",\
"FEATURE_ID","ID","ID_PROOF","KEY_WRAP_AUTH","KEY_ID", \
"MAC_ADDR","MANUFACTURER","MSG_TYPE","MODEL_NAME","MODEL_NUM","","NW_INDEX", \
"NW_KEY","NW_KEY_INDEX","NEW_DEVICE_NAME","NEW_PWD","","OOB_DEV_PWD", \
"OS_VERSION","","POWER_LEVEL","PSK_CURRENT","PSK_MAX","PUBLIC_KEY","RADIO_EN","REBOOT", \
"REG_CURRENT","REG_ESTBLSHD","REG_LIST","REG_MAX","REG_NONCE","REQ_TYPE","RESP_TYPE", \
"RF_BAND","R_HASH1","R_HASH2","R_SNONCE1","R_SNONCE2","SEL_REGISTRAR", \
"SERIAL_NUM","","SimCfg_ST","SSID","TOT_NETWORKS","UUID_E","UUID_R","VENDOR_EXT", \
"VER","X509REQ","X509_CERT","EAP_ID","MSG_COUNT","PUBKEY_HASH", \
"REKEY_KEY","KEY_LIFETIME","PERMCFG_MTHD","SELREG_CFG_MTHD", \
"PRIM_DEV","SEC_DEV_LIST","PORT_DEVICE","AP_SETUP_LOCKED", \
"APP_LIST","EAP_TYPE","","","","","","","INIT_VECTOR","KEY_PROV_AUTO","8021X_EN" \
};

char* msgT[16] = {"","beacon","probeRq","probeRsp","M1","M2","M2D","M3","M4","M5","M6","M7","M8","WSC_ACK","WSC_NACK","WSC_DONE"};
char* assoST[6] = {"Not associated", "Connt success", "Config Failure", "Association Failure","IP failure",""};
char* keyMgmt[8] = {"Open","WPAPSK","Shared","WPA","WPA2","WPA2PSK","",""};
char* encpT[4] = {"None","WEP","TKIP","AES"};
char* cfgErr[20] = {"No Error","OOB Interface Read Error","Decryption CRC Failure","2.4 chan not supported","5.0 chan not supported","Signal too weak","Network auth failure","Network Asso failure","No DHCP repsonse","Failed DHCP config","IP addr conflict","Can't connect to Registrar","Multiple PBC session detected","Rogure acticity suspected","Device busy","Setup locked","Message timeout","Registration Session Timeout","Device Passwd Auth Failure",""};
#endif

#ifdef _MSG_DUMP_
void dumpHexMsg(char* p, uint32 l);
#endif
#ifdef _MSG_TEXT_DUMP_
void dumpTextMsg(char* p, uint32 l);
#endif

#endif // _MSG_ID_H

