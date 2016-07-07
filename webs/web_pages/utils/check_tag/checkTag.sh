#!/bin/sh

tagList="TD OPTION DIV SPAN B"
windowList="alert confirm"

check()
{
	echo -n "."
	echo -e -n "===================== $@ ====================\r\n" >> missing_id.html
	for i in $tagList
	do
		j=`echo $i | tr A-Z a-z` 
		cat $@ | sed 's/<'$j'/<'$i'/g' | sed 's/'$j'>/'$i'>/g' | sed 's/<'$i'[^<>]*>[^<>]*\(\(<[^<>]*>\)\|$\)/\r\n&\r\n/g' | sed -n '/<'$i'\([ \t][^<>]*\)*\\\?>[^<>]*\(<\|$\)/p' | sed 's/\t/ /g' | grep -v '<'$i'[^<>]*>\([^A-Za-z<>]\|\(&nbsp;\)\|\(['$'\'''"] *+.*+ *['$'\'''"]\)\)*\(<\|$\)' | grep -v '<'$i'[^<>]*id *= *\\\?"t_[^<>]*>[^<>]*<\\\?\/'$i'>' >> missing_id.html
	done
	for i in $windowList
	do
		j=`echo $i | tr a-z A-Z`
		cat $@ | sed 's/'$j'/'$i'/g' | sed 's/'$i'[ \t]*(\([^()\n]\|\(([^()\n]*)\)\)*)/\r\n&\r\n/g' | sed -n '/'$i'[ \t]*(/p' | sed 's/\t/ /g' | grep -v $i' *([a-zA-Z0-9_]\+ *= *"' >> missing_id.html
	done
	echo -e -n "\r\n">> missing_id.html
}

doHelp()
{
	echo "check whether all strings are in the right form in which OEM strings could be altered correctly"
	echo "Wrong string will be output to missing_id.html"
	echo "  Useage:"
	echo "	checkTag [filename|directory]"
	exit
}

doVersion()
{
	echo "checkTag Ver 1.0.3"
	echo "Copyright TP-LINK TECHNOLOGIES CO., LTD."
	echo "Author OS-team Wang Wenhao"
}

if [ ! $# -eq 1 ]
then doHelp
fi

if [ $1 == "-h" ]
then doHelp
fi

if [ $1 == "-v" ]
then doVersion
fi

if [ $1 == "-V" ]
then doVersion
fi

if [ $1 == "--help" ]
then doHelp
fi

echo -e -n "\r\n" > missing_id.html
if [ -d $1 ]
then
	fileList=`find $1 -name "*.htm" | sort`
	for k in $fileList
	do
		check $k
	done
else
	if [ -f $1 ]
	then
		if [ ! `echo $1 | grep ".htm"` == "" ]
		then 
			check $1
		else
			echo "$1 Not a htm file"
		fi
	else
		echo "$1 Not a file or a directory"
	fi
fi

echo
echo "done."
