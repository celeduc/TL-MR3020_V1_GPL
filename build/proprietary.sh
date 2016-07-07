#!/bin/bash

# search intermediate products
echo "##### search proprietary..."

if [ -e tftpboot/ap121 ]; then TFTPBOOT_DIR=tftpboot/ap121; fi;
if [ -e tftpboot/ap143 ]; then TFTPBOOT_DIR=tftpboot/ap143; fi;
if [ -e images/ap121 ]; then IMAGE_DIR=images/ap121; fi;
if [ -e images/ap143 ]; then IMAGE_DIR=images/ap143; fi;
if [ -e rootfs.ap121 ]; then ROOTFS_DIR=rootfs.ap121; fi;
if [ -e rootfs.ap143 ]; then ROOTFS_DIR=rootfs.ap143; fi;

intermediate=
if [ -e web_server/archive ]; then intermediate+=" web_server/archive"; else echo "web_server/archive NOT found!"; fi;
if [ -e $TFTPBOOT_DIR/u-boot.bin ]; then intermediate+=" $TFTPBOOT_DIR/u-boot.bin"; else echo "$TFTPBOOT_DIR/u-boot.bin NOT found!"; fi;
if [ -e $TFTPBOOT_DIR/vmlinux.bin ]; then intermediate+=" $TFTPBOOT_DIR/vmlinux.bin"; else echo "$TFTPBOOT_DIR/vmlinux.bin NOT found!"; fi;
if [ -e $TFTPBOOT_DIR/vmlinux.bin.gz ]; then intermediate+=" $TFTPBOOT_DIR/vmlinux.bin.gz"; else echo "$TFTPBOOT_DIR/vmlinux.bin.gz NOT found!"; fi;
if [ -e $IMAGE_DIR/u-boot.bin ]; then intermediate+=" $IMAGE_DIR/u-boot.bin"; else echo "$IMAGE_DIR/u-boot.bin NOT found!"; fi;
if [ -e $IMAGE_DIR/vmlinux ]; then intermediate+=" $IMAGE_DIR/vmlinux"; else echo "$IMAGE_DIR/vmlinux NOT found!"; fi;
if [ -e $IMAGE_DIR/vmlinux.bin ]; then intermediate+=" $IMAGE_DIR/vmlinux.bin"; else echo "$IMAGE_DIR/vmlinux.bin NOT found!"; fi;
if [ -e $IMAGE_DIR/vmlinux.bin.gz ]; then intermediate+=" $IMAGE_DIR/vmlinux.bin.gz"; else echo "$IMAGE_DIR/vmlinux.bin.gz NOT found!"; fi;
if [ -e $IMAGE_DIR/kernel_modules ]; then intermediate+=" $IMAGE_DIR/kernel_modules"; else echo "$IMAGE_DIR/kernel_modules NOT found!"; fi;
if [ -e $IMAGE_DIR/3g ]; then intermediate+=" $IMAGE_DIR/3g"; else echo "$IMAGE_DIR/3g NOT found!"; fi;
if [ -e $IMAGE_DIR/wireless ]; then intermediate+=" $IMAGE_DIR/wireless"; else echo "$IMAGE_DIR/wireless NOT found!"; fi;
if [ -e $ROOTFS_DIR/sbin ]; then intermediate+=" $ROOTFS_DIR/sbin"; else echo "$ROOTFS_DIR/sbin NOT found!"; fi;


echo -e "##### proprietary list:\n $intermediate"

# archive intermediate products
echo "##### pack proprietary..."
tar -czf proprietary.tar.gz $intermediate
echo "##### pack proprietary completed."

