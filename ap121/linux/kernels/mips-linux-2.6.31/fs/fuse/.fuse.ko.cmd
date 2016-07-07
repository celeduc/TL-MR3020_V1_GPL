cmd_fs/fuse/fuse.ko := mips-linux-uclibc-ld -r  -m elf32btsmip  --build-id -o fs/fuse/fuse.ko fs/fuse/fuse.o fs/fuse/fuse.mod.o
