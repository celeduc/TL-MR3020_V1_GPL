cmd_drivers/serial/built-in.o :=  mips-linux-uclibc-ld  -m elf32btsmip   -r -o drivers/serial/built-in.o drivers/serial/serial_core.o drivers/serial/hornet_serial.o 
