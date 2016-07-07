function ipverify(ip_string)
{
    var c;
    var n = 0;
    var ch = ".0123456789"; 
    if (ip_string.length < 7 || ip_string.length > 15)
    return false; 
    for (var i = 0; i < ip_string.length; i++)
    {
        c = ip_string.charAt(i);
        if (ch.indexOf(c) == -1)
        return false; 
        else
        {
            if (c == '.')
            {
                if(ip_string.charAt(i+1) != '.')
                n++; 
                else return false;
            }
        } 
     }
     if (n != 3) 
     return false;
     if (ip_string.indexOf('.') == 0 || ip_string.lastIndexOf('.') == (ip_string.length - 1))
     return false;
     szarray = [0,0,0,0];
     var remain; 
     var i; 
     for(i = 0; i < 3; i++)
     {
        var n = ip_string.indexOf('.');
        szarray[i] = ip_string.substring(0,n);
        remain = ip_string.substring(n+1);
        ip_string = remain; 
     }
     szarray[3] = remain;
     for(i = 0; i < 4; i++)
     {
        if (szarray[i] < 0 || szarray[i] > 255)
        {
            return false; 
        }
    }
    return true; 
}      
function is_ipaddr(ip_string)
{ 
    if(ip_string.length == 0)
    {
        alert("请输入IP地址！"); 
        return false; 
    }
     if (!ipverify(ip_string))
     {
        alert("IP地址输入错误，请重新输入！");
        return false; 
     } 
     return true;
} 
function is_maskaddr(mask_string)
{
    if(mask_string.length == 0)
    {
        alert("请输入子网掩码（例如255.255.255.0）！"); 
        return false; 
    }
    if (!ipverify(mask_string))
    {
        alert("子网掩码输入错误，请重新输入（例如255.255.255.0）！");
        return false;
    }
    return true; 
} 
function is_gatewayaddr(gateway_string)
{
    if(gateway_string.length == 0)
    {
        alert("请输入网关！");
        return false;
    } 
    if (!ipverify(gateway_string))
    {
        alert("网关输入错误，请重新输入！");
        return false;
    }
        return true;
} 
function is_dnsaddr(dns_string)
{ 
    if(dns_string.length == 0)
    {
        alert("请输入DNS服务器（例如202.96.134.133）！"); 
        return false; 
    }
    if (!ipverify(dns_string))
    {
        alert("DNS服务器输入错误，请重新输入（例如202.96.134.133）！"); 
        return false;
    }
    return true;
} 
function macverify(mac_string)
{
        var c; 
        var n = 0;
        var ch = "-0123456789ABCDEFabcdef";
        if (mac_string.length != 17)
        return false;
        for (var i = 0; i < mac_string.length; i++)
        { 
            c = mac_string.charAt(i);
            if (ch.indexOf(c) == -1)
            return false;
            else
            {
                if (c == '-') n++; 
            }
        }
        if (n != 5) 
        return false; 
        for(var i = 2; i < 17; i += 3)
        {
            if (mac_string.charAt(i) != '-') 
            return false; 
        }
        return true;
} 
function is_macaddr(mac_string)
{
    if(mac_string.length == 0)
    {
        alert("请输入MAC地址！");
        return false;
     } 
     if (!macverify(mac_string))
     {
        alert("MAC地址输入错误，请重新输入！");
        return false; 
     } 
     return true; 
}
function is_number(num_string,nMin,nMax)
{
    var c;
    var ch = "0123456789";
    for (var i = 0; i < num_string.length; i++)
    {
        c = num_string.charAt(i); 
        if (ch.indexOf(c) == -1) 
        return false; 
    }
    if(parseInt(num_string) < nMin || parseInt(num_string) > nMax)
    return false;
    return true; 
} 
function lastipverify(lastip,nMin,nMax)
{
    var c;
    var n = 0;
    var ch = "0123456789";
    if(lastip.length = 0) 
    return false; 
    for (var i = 0; i < lastip.length; i++)
    {
        c = lastip.charAt(i);
        if (ch.indexOf(c) == -1) 
        return false; 
    }
    if (parseInt(lastip) < nMin || parseInt(lastip) > nMax)
    return false;
    return true;
} 
function is_lastip(lastip_string,nMin,nMax)
{
    if(lastip_string.length == 0)
    {
        alert("请输入IP地址（1－254）！");
        return false;
    } 
    if (!lastipverify(lastip_string,nMin,nMax))
    {
        alert("IP地址输入错误，请重新输入（1－254）！");
        return false;
    } 
    return true;
} 
function is_domain(domain_string)
{
    var c; var ch = "-.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"; 
    for (var i = 0; i < domain_string.length; i++)
    {
        c = domain_string.charAt(i);
        if (ch.indexOf(c) == -1)
        { 
            alert("输入中含有非法字符，请重新输入！");
            return false; 
        }
    } 
    return true; 
 }
 
function portverify(port_string){
	var c;
	var ch = "0123456789";
	if(port_string.length == 0)
		return false;
	for (var i = 0; i < port_string.length; i++){
		c = port_string.charAt(i);
		if (ch.indexOf(c) == -1)
			return false;
	}
	if (parseInt(port_string) <= 0 || parseInt(port_string) >=65535)
		return false;
	return true;
}
function is_port(port_string)
{
    if(port_string.length == 0)
    {
        alert("请输入端口地址 ( 1-65534 ) ！");
        return false;
    }
    if (!portverify(port_string))
    {
        alert("端口地址输入超出合法范围，请重新输入( 1-65534 ）！"); 
        return false;
    }
        return true;
} 
function charCompare(szname,limit){
	var c;
	var l=0;
	var ch = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@^-_.><,[]{}?/+=|\\'\":;~!#$%()` & ";
	if(szname.length > limit)
		return false;
	for (var i = 0; i < szname.length; i++){
		c = szname.charAt(i);
		if (ch.indexOf(c) == -1){
			l += 2;
		}
		else
		{
			l += 1;
		}
		if ( l > limit)
		{
			return false;
		}
	}
	return true;
}
function is_hostname(name_string, limit){
	if(!charCompare(name_string,limit)){
		alert("您最多只能输入%s个英文字符，一个汉字等于两个英文字符，请重新输入！".replace('%s',limit));
		return false;
	}
	else
		return true;
}
function is_digit(num_string)
{ 
    var c; 
    var ch = "0123456789"; 
    for(var i = 0; i < num_string.length; i++)
    {
        c = num_string.charAt(i); 
        if (ch.indexOf(c) == -1)
        {        
            return false; 
        }
    }
    return true;
}
