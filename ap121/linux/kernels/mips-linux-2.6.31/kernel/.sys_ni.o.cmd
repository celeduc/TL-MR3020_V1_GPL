cmd_kernel/sys_ni.o := mips-linux-uclibc-gcc -Wp,-MD,kernel/.sys_ni.o.d  -nostdinc -isystem /home/project/svn/TL-MR3020_V1_GPL/build/gcc-4.3.3/build_mips/staging_dir/usr/bin/../lib/gcc/mips-linux-uclibc/4.3.3/include -Iinclude  -I/home/project/svn/TL-MR3020_V1_GPL/ap121/linux/kernels/mips-linux-2.6.31/arch/mips/include -include include/linux/autoconf.h -D__KERNEL__ -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -Os -mno-check-zero-division -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -ffreestanding -march=mips32r2 -Wa,-mips32r2 -Wa,--trap -I/home/project/svn/TL-MR3020_V1_GPL/ap121/linux/kernels/mips-linux-2.6.31/arch/mips/include/asm/mach-ar7240 -I/home/project/svn/TL-MR3020_V1_GPL/ap121/linux/kernels/mips-linux-2.6.31/arch/mips/include/asm/mach-generic -D"VMLINUX_LOAD_ADDRESS=0xffffffff80002000" -fno-stack-protector -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow   -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(sys_ni)"  -D"KBUILD_MODNAME=KBUILD_STR(sys_ni)"  -c -o kernel/sys_ni.o kernel/sys_ni.c

deps_kernel/sys_ni.o := \
  kernel/sys_ni.c \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  /home/project/svn/TL-MR3020_V1_GPL/ap121/linux/kernels/mips-linux-2.6.31/arch/mips/include/asm/linkage.h \
  include/linux/errno.h \
  /home/project/svn/TL-MR3020_V1_GPL/ap121/linux/kernels/mips-linux-2.6.31/arch/mips/include/asm/errno.h \
  include/asm-generic/errno-base.h \
  /home/project/svn/TL-MR3020_V1_GPL/ap121/linux/kernels/mips-linux-2.6.31/arch/mips/include/asm/unistd.h \
    $(wildcard include/config/32bit.h) \
    $(wildcard include/config/mips32/o32.h) \
  /home/project/svn/TL-MR3020_V1_GPL/ap121/linux/kernels/mips-linux-2.6.31/arch/mips/include/asm/sgidefs.h \

kernel/sys_ni.o: $(deps_kernel/sys_ni.o)

$(deps_kernel/sys_ni.o):
