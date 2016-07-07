var str_wps_name_long = "Wi-Fi Protected Setup";
var str_wps_name_short = "WPS";
var wlan_wds = 0; //[lixiangkui] 1 =>0, disable wds in portable router for exports

var display_pin_settings = 1;//visable or not.added by tf,101224.

var our_web_site = "www.tp-link.com";
var wireless_ssid_prefix = "TP-LINK_POCKET_3020";
var prompt_net_address = "0";
var default_ip = "192.168.0.254";

var operModeNum = 9;
var minOperMode = 0;
var maxOperMode = operModeNum;
var operModeList = new Array(
//value	enabled	name
	0,		1,			"Access Point",
	1,		0,			"Multi-SSID",
	2,		0,			"Multi-Bss Plus VLAN",
	3,		1,			"Client",
	4,		0,			"WDS Repeater",
	5,		1,			"Repeater",
	6,		0,			"Bridge",
	7,		0,			"Bridge with AP",
	8,		1,			"Bridge with AP",
	9,		0,			"Debug"
);
function getOperModeName(modeIdx)
{
	if(modeIdx<minOperMode || modeIdx>maxOperMode)
	{
		return null;
	}
	if(operModeList[modeIdx*3+1]==0)
	{
		return null;
	}
	else
	{
		return operModeList[modeIdx*3+2];
	}
}

function operModeEnable(modeIdx)
{
	if(modeIdx<minOperMode || modeIdx>maxOperMode)
	{
		return null;
	}
	if(operModeList[modeIdx*3+1]==0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

function getOperModeValue(modeIdx)
{
	if(modeIdx<minOperMode || modeIdx>maxOperMode)
	{
		return null;
	}
	if(operModeList[modeIdx*3+1]==0)
	{
		return null;
	}
	else
	{
		return operModeList[modeIdx*3];
	}
}
function getOperModeIdxByValue(modeValue)
{
	for(var i=0; i<operModeNum; i++)
	{
		if(operModeList[i*3] == modeValue)
			return i;
	}
	return null;
}
