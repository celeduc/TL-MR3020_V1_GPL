#!/bin/bash

######################################################################
# Copyright (C) 2013. Shenzhen TP-LINK Technologies Co. Ltd.
#
# DISCREPTION   : Use envsetup.sh to make build easier.
# AUTHOR        : pax (pangxing@tp-link.net)
# VERSION       : 1.0.0
######################################################################

function help() {
cat <<EOF
Invoke ". build/envsetup.sh" from your shell to add the following functions to your environment:

- chooseproduct                : choose product.
- cleanoutput                  : clean some output dirs.
- getproprietary [JOB_NAME]    : get proprietary from jenkins. HELP: getproprietary -h
- build [OPTION]...            : make, from the top of the tree.
  OPTION args:
    all                        : the same as build.
    --with-clean               : make clean, and make, from the top of the tree.
    --with-clobber             : make clobber, and make, from the top of the tree.
    --debug/-d                 : make with CONFIG_DEBUG.

EOF
}

# choose product.
function chooseproduct()
{
	export PID=

	echo "Choose product (Enter the number):"
	echo "  1. TR863 (TR861 10400E) 1.0"
	echo "  2. TR866 (TR861 5200E) 1.0"
	echo "  3. MR3050 1.0"
	echo "  4. MR3060 1.0"
	echo "  5. MR3020 1.0"
	echo "  6. PW-3G401M 1.0"
	echo "  7. MR3020_Ukraine_MTS 1.0"
	echo "  8. MR3040 2.0"
	echo "  9. Orange3020 1.0"
	echo "  101. MR10U 1.0"
	echo "  112. MR11U 2.0"
	echo "  121. MR12U 1.0"
	echo "  122. MR12U 2.0"
	echo "  131. MR13U 1.0"
	echo "  132. MR13U 2.0"
	echo "  221. MR22U 1.0"
	echo "  7203. WR720N 3.0"
	echo "  7204. WR720N 4.0"
	echo "  m15601. MW156RM3G 1.0"
	echo "  f162c01. FWR162C-3G 1.0"
	echo "  7031. WR703N 1.0"
	echo "  8201. WR820N 1.0"
	echo "  8801. WR880N 1.0"
	echo "  sr8801. SR880N 1.0"
	echo

	local DEFAULT_NUM DEFAULT_VALUE
	DEFAULT_NUM=1
	DEFAULT_VALUE=TR86301
	local ANSWER

	while [ -z $PID ]
	do
		echo -n "Which would you like? "
		if [ -z "$1" ] ; then
			read ANSWER
		else
			echo $1
			ANSWER=$1
		fi
		case $ANSWER in
		1)
			export PID=TR86301
			export BOARD_TYPE=ap121
			;;
		2)
			export PID=TR86601
			export BOARD_TYPE=ap121
			;;
		3)
			export PID=305001
			export BOARD_TYPE=ap121
			;;
		4)
			export PID=306001
			export BOARD_TYPE=ap121
			;;
		5)
			export PID=302001
			export BOARD_TYPE=ap121
			;;
		6)
			export PID=3G401M01
			export BOARD_TYPE=ap121
			;;
		7)
			export PID=302001_ukraine_mts
			export BOARD_TYPE=ap121
			;;
		8)
			export PID=304002
			export BOARD_TYPE=ap121
			;;
		9)
			export PID=Orange302001
			export BOARD_TYPE=ap121
			;;
		101)
			export PID=10U01
			export BOARD_TYPE=ap121
			;;
		112)
			export PID=11U02
			export BOARD_TYPE=ap121
			;;
		121)
			export PID=12U01
			export BOARD_TYPE=ap121
			;;
		122)
			export PID=12U02
			export BOARD_TYPE=ap121
			;;
		131)
			export PID=13U01
			export BOARD_TYPE=ap121
			;;
		132)
			export PID=13U02
			export BOARD_TYPE=ap121
			;;
		221)
			export PID=22U01
			export BOARD_TYPE=ap143
			;;
		7203)
			export PID=72003
			export BOARD_TYPE=ap121
			;;
		7204)
			export PID=72004
			export BOARD_TYPE=ap121
			;;
		m15601)
			export PID=m15601
			export BOARD_TYPE=ap121
			;;
		f162c01)
			export PID=f162c01
			export BOARD_TYPE=ap121
			;;
		7031)
			export PID=70301
			export BOARD_TYPE=ap121
			;;
		8201)
			export PID=82001
			export BOARD_TYPE=ap143
			;;
		8801)
			export PID=88001
			export BOARD_TYPE=ap135_ar8236
			export BUILD_OPTION=_4M
			export dut_type=wr880nv1
			export pid_image_build=wr880nv1_image_cn
			export BOARD_LINK=y
			;;
		sr8801)
			export PID=SR88001
			export BOARD_TYPE=ap135_ar8236
			export BUILD_OPTION=_16M
			export dut_type=sr880nv1
			export pid_image_build=sr880nv1_image_cn
			export BOARD_LINK=y
			;;
		*)
			echo
			echo "I didn't understand your response.  Please try again."
			echo
			;;
		esac
		if [ -n "$1" ] ; then
			break
		fi
	done
}


# project top dir.
export XPROJECT_TOPDIR=`/bin/pwd`

# use our own "gcc -m32" to generate 32bit obj on x86_64 OS.
arch=`uname -m`
if [[ "$arch" == "x86_64" ]]; then
	export PATH=$XPROJECT_TOPDIR/util/xutil:$PATH
fi

# some OS need some set and export
umask 0002
export LD=ld
#export AUTOM4TE=/usr/bin/autom4te

# TIP: You can do more clean here.
# NOTE: If you do cleanoutput, then should do getproprietary.
function cleanoutput()
{
	echo -e "\n\n####### clean some output dirs START #######\n\n"
	echo "rm -Rf $XPROJECT_TOPDIR/images"
	rm -Rf $XPROJECT_TOPDIR/images
	#echo "rm -Rf $XPROJECT_TOPDIR/modules"
	#rm -Rf $XPROJECT_TOPDIR/modules
	echo "rm -Rf $XPROJECT_TOPDIR/rootfs.*"
	rm -Rf $XPROJECT_TOPDIR/rootfs.*
	echo "rm -Rf $XPROJECT_TOPDIR/tftpboot"
	rm -Rf $XPROJECT_TOPDIR/tftpboot
	echo -e "\n\n####### clean some output dirs END  #######\n\n"
}

# WARNING: wget URI may be changed as project. Modify it to right location if needed.
function getproprietary()
{
	local JOB=
	# process args from cmd.
	if [[ "$1" == "-h" ]]; then
		echo "USAGE: getproprietary [JOB_NAME]"
		echo "	1. Use getproprietary after chooseproduct, this will assigned with job uri based on PID implicitly."
		echo "	2. Use getproprietary JOB_NAME, to assign the uri explicitly. eg. getproprietary SR880N"
		return 0
	fi
	if [ ! -z $1 ]; then
		JOB="$1";
	else
		case "$PID" in
# TIP: You can add more product's jenkins job URI here.
		8801)
			JOB="WR880N"
			;;
		SR88001)
			JOB="SR880N"
			;;
		*)
			echo
			echo "envsetup: getproprietary: no job uri assigned!";
			echo
			;;
		esac
	fi

	echo -e "\n\n####### getproprietary START #######\n\n"
	cd $XPROJECT_TOPDIR
	rm -f proprietary.tar.gz*
	wget http://mobileci.rd.tp-link.net/jenkins/job/$JOB/lastSuccessfulBuild/artifact/proprietary.tar.gz
	tar xvzf proprietary.tar.gz
	echo -e "\n\n####### getproprietary END #######\n\n"
}



# global build, this will clean and make whole project, and output log file.
function build()
{
	if [ -z $PID ]; then
		echo "No product is choosed. Please run chooseproduct first!"
		return 1
	else
		echo "Start to build product "$PID"..."
	fi

	PWD_SAVED=`pwd`

	# log path
	LOG_DIR=$XPROJECT_TOPDIR/build/log
	mkdir -p $LOG_DIR
	LOG_FILE="$LOG_DIR/build_"$PID"_`date +%Y%m%d-%H%M%S`.log"

	# cd to build dir.
	cd $XPROJECT_TOPDIR/build

	# start time (recreate log file)
	echo -e "\n\n####### build $PID start at `date` #######\n\n" >> $LOG_FILE

	# for ap135、ap135_ar8236、ap135_hnat、ap135_routing、ap135_wifi、ap136_routing,
	# soft link should be created before build.
	if [[ "$BOARD_LINK" == "y" ]]; then
		ls -1 $XPROJECT_TOPDIR/lsdk | xargs -I {} ln -s $XPROJECT_TOPDIR/ap136 $XPROJECT_TOPDIR/{}
	fi

	# you can define more args judge here.
	local isclobber=n
	local isclean=n
	local isall=n
	local isdebug=n

	# process args from cmd.
	until [ -z "$1" ]
	do
		case "$1" in
		--with-clobber)
			isclobber=y
			;;
		--with-clean)
			isclean=y
			;;
		--debug)
			isdebug=y
			;;
		-d)
			isdebug=y
			;;
		all)
			isall=y
			;;
		*)
			echo
			echo "envsetup: invalid arg: $1, ignored!";
			echo
			;;
		esac
		shift
	done

	# build --debug/-d, export debug flag.
	if [[ "$isdebug" == "y" ]]; then
		export CONFIG_DEBUG=y;
		export CONFIG_BETA=1
		export LOGDEBUG=1
		echo "INFO: Debug flag is: CONFIG_DEBUG=$CONFIG_DEBUG";
	else
		unset CONFIG_DEBUG
		unset CONFIG_BETA
		unset LOGDEBUG
	fi

	# build all, the same as build.
	if [[ "$isall" == "y" ]]; then
		echo "WARNING: \"build all\" is the same as \"build\" now. Do not use it again.";
	fi

	# clean
	# clobber and clean are not compatible, so use "if-else".
	if [[ "$isclobber" == "y" ]]; then
		# for jenkins, we need to clean some directory forcibly before build.
		echo -e "\n\n####### make clobber START #######\n\n" >> $LOG_FILE
		rm -Rf $XPROJECT_TOPDIR/rootfs.*
		rm -Rf $XPROJECT_TOPDIR/tftpboot
		rm -Rf $XPROJECT_TOPDIR/images
		# do more actions here.
		#make clobber 2>&1 | tee -a $LOG_FILE
		echo -e "\n\n####### make clobber END #######\n\n" >> $LOG_FILE
	else if [[ "$isclean" == "y" ]]; then
		echo -e "\n\n####### make clean START #######\n\n" >> $LOG_FILE
		rm -Rf $XPROJECT_TOPDIR/rootfs.*
		rm -Rf $XPROJECT_TOPDIR/tftpboot
		rm -Rf $XPROJECT_TOPDIR/images
		# do more actions here.
		make clean 2>&1 | tee -a $LOG_FILE
		echo -e "\n\n####### make clean END #######\n\n" >> $LOG_FILE
		fi
	fi

	# begin make PID
	echo -e "\n\n####### make pre START #######\n\n" >> $LOG_FILE
	(make fakeroot_build && make toolchain_prep && make fs_prep) 2>&1 | tee -a $LOG_FILE
	echo -e "\n\n####### make pre END #######\n\n" >> $LOG_FILE
	echo -e "\n\n####### make $PID START #######\n\n" >> $LOG_FILE
	(make product_build) 2>&1 | tee -a $LOG_FILE
	echo -e "\n\n####### make $PID END #######\n\n" >> $LOG_FILE

	# for ap135、ap135_ar8236、ap135_hnat、ap135_routing、ap135_wifi、ap136_routing,
	# soft link should be deleted after build.
	#if [[ "$BOARD_LINK" == "y" ]]; then
		#ls -1 $XPROJECT_TOPDIR/lsdk | xargs -I {} unlink $XPROJECT_TOPDIR/{}
	#fi

	# end time
	echo -e "\n\n####### build $PID end at `date` #######\n\n" >> $LOG_FILE

	echo -e "\n\n$PID build log is saved to $LOG_FILE\n\n"

	cd $PWD_SAVED
}


if [ "x$SHELL" != "x/bin/bash" ]; then
	case `ps -o command -p $$` in
		*bash*)
			;;
		*)
			echo "WARNING: Only bash is supported, use of other shell would lead to erroneous results"
			;;
	esac
fi


