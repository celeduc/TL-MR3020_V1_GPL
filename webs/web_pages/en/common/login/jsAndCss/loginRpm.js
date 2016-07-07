var aaa = 'abcdefg';
var code;
function Base64Encoding(input)
{
	var keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	var output = "";
	var chr1, chr2, chr3, enc1, enc2, enc3, enc4;
	var i = 0;

	input = utf8_encode(input);

	while (i < input.length)
	{

		chr1 = input.charCodeAt(i++);
		chr2 = input.charCodeAt(i++);
		chr3 = input.charCodeAt(i++);

		enc1 = chr1 >> 2;
		enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
		enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
		enc4 = chr3 & 63;

		if (isNaN(chr2)) {
			enc3 = enc4 = 64;
		} else if (isNaN(chr3)) {
			enc4 = 64;
		}

		output = output +
		keyStr.charAt(enc1) + keyStr.charAt(enc2) +
		keyStr.charAt(enc3) + keyStr.charAt(enc4);

	}

	return output;
}

function utf8_encode (string)
{
	string = string.replace(/\r\n/g,"\n");
	var utftext = "";

	for (var n = 0; n < string.length; n++) {

		var c = string.charCodeAt(n);

		if (c < 128) {
			utftext += String.fromCharCode(c);
		}
		else if((c > 127) && (c < 2048)) {
			utftext += String.fromCharCode((c >> 6) | 192);
			utftext += String.fromCharCode((c & 63) | 128);
		}
		else {
			utftext += String.fromCharCode((c >> 12) | 224);
			utftext += String.fromCharCode(((c >> 6) & 63) | 128);
			utftext += String.fromCharCode((c & 63) | 128);
		}

	}

	return utftext;
}

function getCookie()
{
	var cookieLoginTime;
	var startIndex;
	var endIndex;
	var times = 1;

	startIndex = document.cookie.indexOf("TPLoginTimes=");

	if (-1 != startIndex)
	{
		cookieLoginTime = document.cookie.substring(startIndex);
		startIndex = cookieLoginTime.indexOf("=");
		cookieLoginTime = cookieLoginTime.substring(startIndex +1);
		endIndex = cookieLoginTime.indexOf(";");

		if ( -1 == endIndex)
		{
			times = parseInt(cookieLoginTime);
			times = times + 1;
		}
		else
		{
			startIndex = 0;
			cookieLoginTime = cookieLoginTime.substring(startIndex, endIndex);
			times = parseInt(cookieLoginTime);
			times = times + 1;
		}

		if (times == 5)
		{
			times = 1;
		}
		document.cookie = "TPLoginTimes="+ times;
	}
	else
	{
		document.cookie = "TPLoginTimes="+ times;
	}
}

function PCWin(event)
{
	if (event.keyCode == 13)
	{
		var auth;
		var strtemp = location.href;
		var admin = document.getElementById("pcAdmin").value;
		var password = document.getElementById("pcPassword").value;
		auth = "Basic "+Base64Encoding(admin+":"+password);
		document.cookie = "Authorization="+escape(auth)+";path=/";
		document.cookie = "subType=pcSub";
		getCookie();
		//location.reload();
		location.href = strtemp;
	}
}
function PCSubWin()
{
	var auth;
	var strtemp = location.href;
	var admin = document.getElementById("pcAdmin").value;
	var password = document.getElementById("pcPassword").value;
	auth = "Basic "+Base64Encoding(admin+":"+password);
	document.cookie = "Authorization="+escape(auth)+";path=/";
	document.cookie = "subType=pcSub";
	getCookie();
	//location.reload();
	location.href = strtemp;
}

function Win(buttonId)
{
	var auth;
	var admin = document.getElementById("admin").value;
	var password = document.getElementById("password").value;
	auth = "Basic "+Base64Encoding(admin+":"+password);
	document.cookie = "Authorization="+auth;
	document.cookie = "subType="+buttonId;
	getCookie();
	location.reload();
}
var isShowReset = false;
function showResetMsgPh()
{
	isShowReset = !isShowReset;
	$("resetMsgPh").style.display = (isShowReset)?"block":"none";
	$("forPwdPh").style.display = (isShowReset)?"none":"block";
}
function showResetMsg()
{
	isShowReset = !isShowReset;
	$("resetMsg").style.display = (isShowReset)?"block":"none";
	$("forPwd").style.display = (isShowReset)?"none":"block";
}
function $(id)
{
	return document.getElementById(id);
}
function pageLoad()
{
		$("panelThre").style.display = "block";
		document.body.style.backgroundColor="white";
		document.cookie = "Authorization=;path=/";
		var ErrNum = httpAutErrorArray[0];
		isDefaultName?$("pcPassword").focus():$("pcAdmin").focus();
}