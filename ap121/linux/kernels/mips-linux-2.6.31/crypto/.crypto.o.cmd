cmd_crypto/crypto.o := mips-linux-uclibc-ld  -m elf32btsmip   -r -o crypto/crypto.o crypto/api.o crypto/cipher.o crypto/digest.o crypto/compress.o 
