cmd_fs/fat/fat.ko := mips-linux-uclibc-ld -r  -m elf32btsmip  --build-id -o fs/fat/fat.ko fs/fat/fat.o fs/fat/fat.mod.o
