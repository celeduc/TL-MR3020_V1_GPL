#!/bin/sh

FLAG=cn
PWDT=`pwd`
get()
{
	for k in $@
	do 
		if [ ! -d $k ]
		then 
			echo $k not exist or is not a dir!
		else	
			DIRNAME=`echo $k | sed 's/\// /g' | awk '{print $NF}'`
			echo target dir $DIRNAME\_$FLAG
			
			echo making header
			echo -e -n "[FOLDER_PATH]\r\n" > $DIRNAME\_$FLAG.ini
			echo -e -n "DEST_PATH=/tmp/\r\n" >> $DIRNAME\_$FLAG.ini
			echo -e -n "SOURCE_PATH=/mnt/$k/\r\n" >> $DIRNAME\_$FLAG.ini
			echo -e -n "INTER_PATH=root/rc_filesys/doc/\r\n" >> $DIRNAME\_$FLAG.ini
			echo -e -n "[TRANS_RULE]\r\n" >> $DIRNAME\_$FLAG.ini
			echo -e -n "RULE=big\r\n" >> $DIRNAME\_$FLAG.ini
			echo -e -n "START=TP_C_TAG\r\n" >> $DIRNAME\_$FLAG.ini
			echo -e -n "[FILE_NAME]\r\n" >> $DIRNAME\_$FLAG.ini
			
			echo add file to ini list
			DIRLIST="userRpm help dynaform frames images localiztion"
			for j in $DIRLIST
			do
				if [ -d $k/$j ]
				then
					cd $k
					FILELIST=`find $j -name "*.*"`
					cd $PWDT
					for i in $FILELIST
					do
						echo -e -n "RELATIVE_NAME=$i\r\n" >> $DIRNAME\_$FLAG.ini
					done
					echo -e -n "\r\n" >> $DIRNAME\_$FLAG.ini
				else
					echo $k/$j not exist or is not a dir!
				fi
			done
			echo
			echo -e -n "[INDEPENDENT_FILE]\r\n" >> $DIRNAME\_$FLAG.ini
		fi
		echo
	done
	echo done
}

get cn/common
get cn/oem/*
FLAG=en
get en/common
get en/oem/*