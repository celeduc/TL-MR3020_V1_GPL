cmd_crypto/crypto_hash.o := mips-linux-uclibc-ld  -m elf32btsmip   -r -o crypto/crypto_hash.o crypto/hash.o crypto/ahash.o crypto/shash.o 
