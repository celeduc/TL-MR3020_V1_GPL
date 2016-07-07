cmd_drivers/scsi/built-in.o :=  mips-linux-uclibc-ld  -m elf32btsmip   -r -o drivers/scsi/built-in.o drivers/scsi/scsi_mod.o drivers/scsi/sd_mod.o drivers/scsi/sg.o 
