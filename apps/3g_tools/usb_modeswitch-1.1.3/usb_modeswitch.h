/*
  This file is part of usb-modeswitch, a mode switching tool for controlling
  flip flop (multiple device) USB gear

  Version 1.1.3, 2010/06/21
  Copyright (C) 2007, 2008, 2009, 2010  Josua Dietze

  Config file parsing stuff borrowed from Guillaume Dargaud
  (http://www.gdargaud.net/Hack/SourceCode.html)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details:

  http://www.gnu.org/licenses/gpl.txt

*/


#include <stdlib.h>
#include <usb.h>

void readConfigFile(const char *configFilename);
void printConfig();
int switchSendMessage();
int switchConfiguration();
int switchAltSetting();
void switchHuaweiMode();
void switchSierraMode();
void switchGCTMode();
void switchAVMMode();
int switchSonyMode();
int detachDriver();
int checkSuccess();
int sendMessage(char* message, int count);
int write_bulk(int endpoint, char *message, int length);
int read_bulk(int endpoint, char *buffer, int length);
void release_usb_device(int dummy);
struct usb_device* search_devices( int *numFound, int vendor, int product, char* productList, int targetClass, int mode);
int find_first_bulk_output_endpoint(struct usb_device *dev);
int find_first_bulk_input_endpoint(struct usb_device *dev);
char* ReadParseParam(const char* FileName, char *VariableName);
int hex2num(char c);
int hex2byte(const char *hex);
int hexstr2bin(const char *hex, char *buffer, int len);
void printVersion();
void printHelp();
int readArguments(int argc, char **argv);
void deviceDescription();
int deviceInquire();
void resetUSB();
void release_usb_device(int dummy);

// Boolean
#define  and     &&
#define  or      ||
#define  not     !

// Bitwise
#define  bitand  &
#define  bitor   |
#define  compl   ~
#define  xor     ^

// Equals
#define  and_eq  &=
#define  not_eq  !=
#define  or_eq   |=
#define  xor_eq  ^=

extern char* ReadParseParam(const char* FileName, char *VariableName);

extern char *TempPP;

#define ParseParamString(ParamFileName, Str) \
	if ((TempPP=ReadParseParam((ParamFileName), #Str))!=NULL) \
		strcpy(Str, TempPP); else Str[0]='\0'
		
#define ParseParamInt(ParamFileName, Int) \
	if ((TempPP=ReadParseParam((ParamFileName), #Int))!=NULL) \
		Int=atoi(TempPP)

#define ParseParamHex(ParamFileName, Int) \
	if ((TempPP=ReadParseParam((ParamFileName), #Int))!=NULL) \
		Int=strtol(TempPP, NULL, 16)

#define ParseParamFloat(ParamFileName, Flt) \
	if ((TempPP=ReadParseParam((ParamFileName), #Flt))!=NULL) \
		Flt=atof(TempPP)

#define ParseParamBool(ParamFileName, B) \
	if ((TempPP=ReadParseParam((ParamFileName), #B))!=NULL) \
		B=(toupper(TempPP[0])=='Y' || toupper(TempPP[0])=='T'|| TempPP[0]=='1'); else B=0

