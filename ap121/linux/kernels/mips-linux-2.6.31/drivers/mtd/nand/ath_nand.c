/*
 * linux/drivers/mtd/nand/ath_nand.c
 * vim: tabstop=8 : noexpandtab
 * Derived from alauda.c
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/bitops.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/cacheflush.h>
#include <atheros.h>

#define DRV_NAME	"ath-nand"
#define DRV_VERSION	"0.1"
#define DRV_AUTHOR	"Atheros"
#define DRV_DESC	"Atheros on-chip NAND FLash Controller Driver"

#define ATH_NAND_FLASH_BASE	0x1b000000u
#define ATH_NF_RST		(ATH_NAND_FLASH_BASE + 0x200u)
#define ATH_NF_CTRL		(ATH_NAND_FLASH_BASE + 0x204u)
#define ATH_NF_RST_REG		(ATH_NAND_FLASH_BASE + 0x208u)
#define ATH_NF_ADDR0_0		(ATH_NAND_FLASH_BASE + 0x21cu)
#define ATH_NF_ADDR0_1		(ATH_NAND_FLASH_BASE + 0x224u)
#define ATH_NF_DMA_ADDR		(ATH_NAND_FLASH_BASE + 0x264u)
#define ATH_NF_DMA_COUNT	(ATH_NAND_FLASH_BASE + 0x268u)
#define ATH_NF_DMA_CTRL		(ATH_NAND_FLASH_BASE + 0x26cu)
#define ATH_NF_MEM_CTRL		(ATH_NAND_FLASH_BASE + 0x280u)
#define ATH_NF_PG_SIZE		(ATH_NAND_FLASH_BASE + 0x284u)
#define ATH_NF_RD_STATUS	(ATH_NAND_FLASH_BASE + 0x288u)
#define ATH_NF_TIMINGS_ASYN	(ATH_NAND_FLASH_BASE + 0x290u)

#define ATH_NF_BLK_SIZE_S	0x11
#define ATH_NF_BLK_SIZE		(1 << ATH_NF_BLK_SIZE_S)	//Number of Pages per block; 0=32, 1=64, 2=128, 3=256
#define ATH_NF_BLK_SIZE_M	(ATH_NF_BLK_SIZE - 1)
#define ATH_NF_PAGE_SIZE	2048	//No of bytes per page; 0=256, 1=512, 2=1024, 3=2048, 4=4096, 5=8182, 6= 16384, 7=0
#define ATH_NF_CUSTOM_SIZE_EN	0x1	//1 = Enable, 0 = Disable
#define ATH_NF_ADDR_CYCLES_NUM	0x5	//No of Address Cycles
#define ATH_NF_TIMING_ASYN	0x0
#define ATH_NF_STATUS_OK	0xc0
#define ATH_NF_RD_STATUS_MASK	0xc7

#define ATH_NAND_IO_DBG		0
#define ATH_NAND_OOB_DBG	0
#define ATH_NAND_IN_DBG		0

#if ATH_NAND_IO_DBG
#	define iodbg	printk
#else
#	define iodbg(...)
#endif

#if ATH_NAND_OOB_DBG
#	define oobdbg	printk
#else
#	define oobdbg(...)
#endif

#if ATH_NAND_IN_DBG
#	define indbg(a, ...)					\
	do {							\
		printk("--- %s(%d):" a "\n",			\
			__func__, __LINE__, ## __VA_ARGS__);	\
	} while (0)
#else
#	define indbg(...)
#	define indbg1(a, ...)					\
	do {							\
		printk("--- %s(%d):" a "\n",			\
			__func__, __LINE__, ## __VA_ARGS__);	\
	} while (0)
#endif

/*
 * Data structures for ath nand flash controller driver
 */

typedef union {
	uint8_t			byte_id[8];

	struct {
		uint8_t		sa1	: 1,	// Serial access time (bit 1)
				org	: 1,	// Organisation
				bs	: 2,	// Block size
				sa0	: 1,	// Serial access time (bit 0)
				ss	: 1,	// Spare size per 512 bytes
				ps	: 2,	// Page Size

				wc	: 1,	// Write Cache
				ilp	: 1, 	// Interleaved Programming
				nsp	: 2, 	// No. of simult prog pages
				ct	: 2,	// Cell type
				dp	: 2,	// Die/Package

				did,		// Device id
				vid,		// Vendor id

				res1	: 2,	// Reserved
				pls	: 2,	// Plane size
				pn	: 2,	// Plane number
				res2	: 2;	// Reserved
	} __details;
} ath_nand_id_t;

uint64_t ath_plane_size[] = {
	64 << 20,
	 1 << 30,
	 2 << 30,
	 4 << 30,
	 8 << 30
};


/* ath nand info */
typedef struct {
	/* mtd info */
	struct nand_hw_control	controller;
	struct mtd_info		mtd;

	/* platform info */
	unsigned short		page_size,
				data_width;

	/* NAND MTD partition information */
	int			nr_partitions;
	struct mtd_partition	*partitions;

	/* DMA stuff */
	struct completion	dma_completion;

	unsigned		ba0,
				ba1,
				cmd;	// Current command
	ath_nand_id_t		__id;	// for readid
	uint8_t			*buf;
} ath_nand_sc_t;

ath_nand_sc_t *ath_nand_sc;

#define	id	__id.__details
#define	bid	__id.byte_id

static const char *part_probes[] __initdata = { "cmdlinepart", "RedBoot", NULL };

static unsigned
ath_nand_status(void)
{
	unsigned	rddata;

	rddata = ath_reg_rd(ATH_NF_RST_REG);
	while (rddata != 0xff) {
		rddata = ath_reg_rd(ATH_NF_RST_REG);
	}

	ath_reg_wr(ATH_NF_RST, 0x07024);	// READ STATUS
	rddata = ath_reg_rd(ATH_NF_RD_STATUS);

	return rddata;
}

static unsigned
ath_nand_rw_page(int rd, unsigned addr0, unsigned addr1, unsigned count, unsigned char *buf)
{
	unsigned	rddata;
	char		*err[] = { "Write", "Read" };

	ath_reg_wr(ATH_NF_ADDR0_0, addr0);
	ath_reg_wr(ATH_NF_ADDR0_1, addr1);
	ath_reg_wr(ATH_NF_DMA_ADDR, (unsigned)buf);
	ath_reg_wr(ATH_NF_DMA_COUNT, count);
	ath_reg_wr(ATH_NF_PG_SIZE, count);

	if (rd) {	// Read Page
		ath_reg_wr(ATH_NF_DMA_CTRL, 0xcc);
		ath_reg_wr(ATH_NF_RST, 0x30006a);
	} else {	// Write Page
		ath_reg_wr(ATH_NF_DMA_CTRL, 0x8c);
		ath_reg_wr(ATH_NF_RST, 0x10804c);
	}

	rddata = ath_nand_status() & ATH_NF_RD_STATUS_MASK;
	if (rddata != ATH_NF_STATUS_OK) {
		printk("%s: %s Failed. status = 0x%x\n", __func__, err[rd], rddata);
	}

	return rddata;
}

void
ath_nand_dump_buf(loff_t addr, void *v, unsigned count)
{
	unsigned	*buf = v,
			*end = buf + (count / sizeof(*buf));

	iodbg("____ Dumping %d bytes at 0x%p 0x%llx_____\n", count, buf, addr);

	for (; buf < end; buf += 4) {
		iodbg("%p: %08x %08x %08x %08x\n",
			buf, buf[0], buf[1], buf[2], buf[3]);
	}
	iodbg("___________________________________\n");
	//while(1);
}


/* max page size + oob buf size */
uint8_t	ath_nand_io_buf[4096 + 256];

static int
ath_nand_rw_buff(struct mtd_info *mtd, int rd, uint8_t *buf,
		loff_t addr, size_t len, size_t *iodone)
{
	unsigned	iolen, ret = ATH_NF_STATUS_OK;
	unsigned char	*pa;

	*iodone = 0;

	while (len) {
		unsigned b, p, c, ba0, ba1;

		b = (addr >> mtd->erasesize_shift);
		p = (addr & mtd->erasesize_mask) >> mtd->writesize_shift;
		c = (addr & mtd->writesize_mask);

		/*
		 * addr format:
		 * a0 - a11 - xxxx - a19 - a27 == 32 bits, will be in ba0
		 * a28 - a31 - xxxxxxxxxxxxxxxx == 4 bits, will be in ba1 in lsb
		 */

		ba0 = (b << 22) | (p << 16);
		ba1 = (b >>  9) & 0xf;
		if (c) {
			iolen = mtd->writesize - c;
		} else {
			iolen = mtd->writesize;
		}

		if (len < iolen) {
			iolen = len;
		}

		if (!rd) {
			/* FIXME for writes FIXME */
			memcpy(ath_nand_io_buf, buf, iolen);
		}

		pa = (unsigned char *)dma_map_single(NULL, ath_nand_io_buf,
					mtd->writesize, DMA_BIDIRECTIONAL);

		//	iodbg("%s(%c): 0x%x 0x%x 0x%x 0x%p\n", __func__,
		//		rd ? 'r' : 'w', ba0, ba1, iolen, pa);

		ret = ath_nand_rw_page(rd, ba0, ba1, mtd->writesize, pa);

		dma_unmap_single(NULL, (dma_addr_t)pa, mtd->writesize,
					DMA_BIDIRECTIONAL);

		if (rd) {
			memcpy(buf, ath_nand_io_buf + c, iolen);
		}

		//	ath_nand_dump_buf(addr, buf, iolen);

		if (ret != ATH_NF_STATUS_OK) {
			return 1;
		}

		len -= iolen;
		buf += iolen;
		addr += iolen;
		*iodone += iolen;
	}

	return 0;
}

static int
ath_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	int	ret;

	if (!len || !retlen) return (0);

	indbg("0x%llx	%u", to, len);

	ret = ath_nand_rw_buff(mtd, 0 /* write */, (u_char *)buf, to, len, retlen);

	return ret;
}

static int
ath_nand_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	int	ret;

	if (!len || !retlen) return (0);

	indbg("0x%llx	%u", from, len);

	ret = ath_nand_rw_buff(mtd, 1 /* read */, buf, from, len, retlen);

	return ret;
}

static inline void
ath_nand_block_erase(unsigned addr0, unsigned addr1)
{
	unsigned	rddata;

	indbg("0x%x 0x%x", addr1, addr0);

	ath_reg_wr(ATH_NF_ADDR0_0, addr0);
	ath_reg_wr(ATH_NF_ADDR0_1, addr1);
	ath_reg_wr(ATH_NF_RST, 0xd0600e);	// BLOCK ERASE

	rddata = ath_nand_status() & ATH_NF_RD_STATUS_MASK;
	if (rddata != ATH_NF_STATUS_OK) {
		printk("Erase Failed. status = 0x%x", rddata);
	}
}


static int
ath_nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	uint64_t s_first, s_last, i, j;

	if (instr->addr + instr->len > mtd->size) {
		return (-EINVAL);
	}

	s_first = instr->addr;
	s_last = s_first + instr->len;

	indbg("0x%llx 0x%llx 0x%x", instr->addr, s_last, mtd->erasesize);

	for (j = 0, i = s_first; i <= s_last; j++, i += mtd->erasesize) {
		ulong b, ba0, ba1;

		b = (i >> mtd->erasesize_shift);

		ba0 = (b << 22);
		ba1 = (b >>  9) & 0xf;

		ath_nand_block_erase(ba0, ba1);

	}

	if (instr->callback) {
		instr->state |= MTD_ERASE_DONE;
		instr->callback (instr);
	}

	return 0;
}

static int
ath_nand_rw_oob(struct mtd_info *mtd, int rd, loff_t addr,
		struct mtd_oob_ops *ops)
{
	unsigned	ret = ATH_NF_STATUS_OK;
	unsigned char	*pa;
	unsigned	b, p, c, ba0, ba1;

	b = (addr >> mtd->erasesize_shift);
	p = (addr & mtd->erasesize_mask) >> mtd->writesize_shift;
	c = (addr & mtd->writesize_mask);

	ba0 = (b << 22) | (p << 16);
	ba1 = (b >>  9) & 0xf;

	if (!rd) {
		if (ops->datbuf) {
			/*
			 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
			 * We assume that the caller gives us a full
			 * page to write. We don't read the page and
			 * update the changed portions alone.
			 *
			 * Hence, not checking for len < or > pgsz etc...
			 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
			 */
			memcpy(ath_nand_io_buf, ops->datbuf, ops->len);
		}
		memcpy(ath_nand_io_buf + mtd->writesize, ops->oobbuf, ops->ooblen);
	}

	pa = (unsigned char *)dma_map_single(NULL, ath_nand_io_buf,
				mtd->writesize + mtd->oobsize, DMA_BIDIRECTIONAL);

	//	oobdbg("%s(%c): 0x%x 0x%x 0x%x 0x%p\n", __func__,
	//		rd ? 'r' : 'w', ba0, ba1, iolen, pa);

	ret = ath_nand_rw_page(rd, ba0, ba1, mtd->writesize + mtd->oobsize, pa);

	dma_unmap_single(NULL, (dma_addr_t)pa, mtd->writesize + mtd->oobsize,
				DMA_BIDIRECTIONAL);

	//	ath_nand_dump_buf(addr, buf, iolen);

	if (ret != ATH_NF_STATUS_OK) {
		return 1;
	}

	if (rd) {
		if (ops->datbuf) {
			memcpy(ops->datbuf, ath_nand_io_buf, ops->len);
		}
		memcpy(ops->oobbuf, ath_nand_io_buf + mtd->writesize, ops->ooblen);
	}

	if (ops->datbuf) {
		ops->retlen = ops->len;
	}
	ops->oobretlen = ops->ooblen;

	return 0;
}

static int
ath_nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	oobdbg(	"%s: from: 0x%llx\n"
		"mode: 0x%x\n"
		"len: 0x%x\n"
		"retlen: 0x%x\n"
		"ooblen: 0x%x\n"
		"oobretlen: 0x%x\n"
		"ooboffs: 0x%x\n"
		"datbuf: %p\n"
		"oobbuf: %p\n", __func__, from,
		ops->mode, ops->len, ops->retlen, ops->ooblen,
		ops->oobretlen, ops->ooboffs, ops->datbuf,
		ops->oobbuf);

	indbg("0x%llx %p %p %u", from, ops->oobbuf, ops->datbuf, ops->len);

	return ath_nand_rw_oob(mtd, 1 /* read */, from, ops);
}

static int
ath_nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	oobdbg(	"%s: from: 0x%llx\n"
		"mode: 0x%x\n"
		"len: 0x%x\n"
		"retlen: 0x%x\n"
		"ooblen: 0x%x\n"
		"oobretlen: 0x%x\n"
		"ooboffs: 0x%x\n"
		"datbuf: %p\n"
		"oobbuf: %p\n", __func__, to,
		ops->mode, ops->len, ops->retlen, ops->ooblen,
		ops->oobretlen, ops->ooboffs, ops->datbuf,
		ops->oobbuf);

	indbg("0x%llx", to);

	return ath_nand_rw_oob(mtd, 0 /* write */, to, ops);
}

static int
ath_nand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	indbg("unimplemented 0x%llx", ofs);
	return 0;
}

static int
ath_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	indbg("unimplemented 0x%llx", ofs);
	return 0;
}

static int __devinit
ath_parse_read_id(ath_nand_sc_t *sc)
{
	printk(	"____ %s _____\n"
		"vid	: 0x%x\n"
		"did	: 0x%x\n"
		"wc	: 0x%x\n"
		"ilp	: 0x%x\n"
		"nsp	: 0x%x\n"
		"ct	: 0x%x\n"
		"dp	: 0x%x\n"
		"sa1	: 0x%x\n"
		"org	: 0x%x\n"
		"bs	: 0x%x\n"
		"sa0	: 0x%x\n"
		"ss	: 0x%x\n"
		"ps	: 0x%x\n"
		"res1	: 0x%x\n"
		"pls	: 0x%x\n"
		"pn	: 0x%x\n"
		"res2	: 0x%x\n"
		"-------------\n", __func__,
			sc->id.vid, sc->id.did, sc->id.wc, sc->id.ilp,
			sc->id.nsp, sc->id.ct, sc->id.dp, sc->id.sa1,
			sc->id.org, sc->id.bs, sc->id.sa0, sc->id.ss,
			sc->id.ps, sc->id.res1, sc->id.pls, sc->id.pn,
			sc->id.res2);
	return 0;
}

/*
 * System initialization functions
 */
static int __devinit
ath_nand_hw_init(ath_nand_sc_t *sc)
{
	unsigned char		*pa;

	// Control Reg Setting
	ath_reg_wr(ATH_NF_CTRL,	(ATH_NF_ADDR_CYCLES_NUM) |
					(ATH_NF_BLK_SIZE << 6) |
					(ATH_NF_PAGE_SIZE << 8) |
					(ATH_NF_CUSTOM_SIZE_EN << 11));

	// TIMINGS_ASYN Reg Settings
	ath_reg_wr(ATH_NF_TIMINGS_ASYN, ATH_NF_TIMING_ASYN);

	// NAND Mem Control Reg
	ath_reg_wr(ATH_NF_MEM_CTRL, 0xff00);

	// Reset Command
	ath_reg_wr(ATH_NF_RST, 0xff00);

	pa = (unsigned char *)dma_map_single(NULL, &sc->id,
					8, DMA_BIDIRECTIONAL);
	ath_reg_wr(ATH_NF_DMA_ADDR, (unsigned)virt_to_phys(&sc->id));
	ath_reg_wr(ATH_NF_ADDR0_0, 0x0);
	ath_reg_wr(ATH_NF_ADDR0_1, 0x0);
	ath_reg_wr(ATH_NF_DMA_COUNT, 0x8);
	ath_reg_wr(ATH_NF_PG_SIZE, 0x8);
	ath_reg_wr(ATH_NF_DMA_CTRL, 0xcc);
	ath_reg_wr(ATH_NF_RST, 0x9061);	// READ ID

	ath_nand_status();	// Intentionally not checking return value
	dma_unmap_single(NULL, (dma_addr_t)pa, 8, DMA_BIDIRECTIONAL);
	printk("Ath Nand ID[%p]: %02x:%02x:%02x:%02x:%02x\n",
			sc->bid, sc->bid[0], sc->bid[1],
			sc->bid[2], sc->bid[3], sc->bid[4]);
	ath_parse_read_id(sc);

	iodbg("******* %s done ******\n", __func__);

	return 0;
}

/*
 * Device management interface
 */
static int __devinit ath_nand_add_partition(ath_nand_sc_t *sc)
{
	struct mtd_info *mtd = &sc->mtd;

#ifdef CONFIG_MTD_PARTITIONS
	sc->nr_partitions = parse_mtd_partitions(mtd, part_probes,
						 &sc->partitions, 0);
	return add_mtd_partitions(mtd, sc->partitions, sc->nr_partitions);
#else
	return add_mtd_device(mtd);
#endif
}

static int __devexit ath_nand_remove(void)
{
#ifdef CONFIG_MTD_PARTITIONS
	/* Deregister partitions */
	del_mtd_partitions(&ath_nand_sc->mtd);
#endif
	kfree(ath_nand_sc);
	ath_nand_sc = NULL;
	return 0;
}

// Copied from drivers/mtd/onenand/onenand_base.c
/**
 *  onenand_oob_128 - oob info for Flex-Onenand with 4KB page
 *  For now, we expose only 64 out of 80 ecc bytes
 */
static struct nand_ecclayout onenand_oob_128 = {
	.eccbytes	= 64,
	.oobavail	= 128,
	.eccpos		= {
		6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		102, 103, 104, 105
		},
	.oobfree	= {
		{2, 4}, {18, 4}, {34, 4}, {50, 4},
		{66, 4}, {82, 4}, {98, 4}, {114, 4}
	}
};

/**
 * onenand_oob_64 - oob info for large (2KB) page
 */
static struct nand_ecclayout onenand_oob_64 = {
	.eccbytes	= 20,
	.oobavail	= 64,
	.eccpos		= {
		8, 9, 10, 11, 12,
		24, 25, 26, 27, 28,
		40, 41, 42, 43, 44,
		56, 57, 58, 59, 60,
		},
	.oobfree	= {
		{2, 3}, {14, 2}, {18, 3}, {30, 2},
		{34, 3}, {46, 2}, {50, 3}, {62, 2}
	}
};

/**
 * onenand_oob_32 - oob info for middle (1KB) page
 */
static struct nand_ecclayout onenand_oob_32 = {
	.eccbytes	= 10,
	.oobavail	= 32,
	.eccpos		= {
		8, 9, 10, 11, 12,
		24, 25, 26, 27, 28,
		},
	.oobfree	= { {2, 3}, {14, 2}, {18, 3}, {30, 2} }
};

/*
 * ath_nand_probe
 *
 * called by device layer when it finds a device matching
 * one our driver can handled. This code checks to see if
 * it can allocate all necessary resources then calls the
 * nand layer to look for devices
 */
static int __devinit ath_nand_probe(void)
{
	ath_nand_sc_t *sc = NULL;
	struct mtd_info *mtd = NULL;
	int err = 0;

	sc = kzalloc(sizeof(*sc), GFP_KERNEL);
	if (sc == NULL) {
		printk("%s: no memory for flash sc\n", __func__);
		err = -ENOMEM;
		goto out_err_kzalloc;
	}

	spin_lock_init(&sc->controller.lock);
	init_waitqueue_head(&sc->controller.wq);

	/* initialise the hardware */
	err = ath_nand_hw_init(sc);
	if (err) {
		goto out_err_hw_init;
	}

	/* initialise mtd sc data struct */
	mtd = &sc->mtd;

	mtd->name		= DRV_NAME;
	mtd->owner		= THIS_MODULE;
	mtd->size		= ath_plane_size[sc->id.pls] << sc->id.pn;

	mtd->writesize_shift	= 10 + sc->id.ps;
	mtd->writesize		= (1 << mtd->writesize_shift);
	mtd->writesize_mask	= (mtd->writesize - 1);

	mtd->erasesize_shift	= 16 + sc->id.bs;
	mtd->erasesize		= (1 << mtd->erasesize_shift);
	mtd->erasesize_mask	= (mtd->erasesize - 1);

	mtd->oobsize		= (mtd->writesize / 512) * (8 << sc->id.ss);
	mtd->oobavail		= mtd->oobsize;

	mtd->type		= MTD_NANDFLASH;
	mtd->flags		= MTD_CAP_NANDFLASH;

	mtd->read		= ath_nand_read;
	mtd->write		= ath_nand_write;
	mtd->erase		= ath_nand_erase;

	mtd->read_oob		= ath_nand_read_oob;
	mtd->write_oob		= ath_nand_write_oob;

	mtd->block_isbad	= ath_nand_block_isbad;
	mtd->block_markbad	= ath_nand_block_markbad;

	mtd->priv		= sc;

	if (mtd->oobsize == 128) {
		mtd->ecclayout	= &onenand_oob_128;
	} else if (mtd->oobsize == 64) {
		mtd->ecclayout	= &onenand_oob_64;
	} else if (mtd->oobsize == 32) {
		mtd->ecclayout	= &onenand_oob_32;
	} else {
		mtd->ecclayout	= NULL;
	}

	/* add NAND partition */
	ath_nand_add_partition(sc);

	ath_nand_sc = sc;

	return 0;

out_err_hw_init:
	kfree(sc);
out_err_kzalloc:

	return err;
}

#if 0
static struct platform_driver ath_nand_driver = {
	//.probe		= ath_nand_probe,
	.remove		= __exit_p(ath_nand_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};
#endif

static int __init ath_nand_init(void)
{
	printk(DRV_DESC ", Version " DRV_VERSION
		" (c) 2010 Atheros Communications, Ltd.\n");

	//return platform_driver_register(&ath_nand_driver);
	//return platform_driver_probe(&ath_nand_driver, ath_nand_probe);
	return ath_nand_probe();
}

static void __exit ath_nand_exit(void)
{
	//platform_driver_unregister(&ath_nand_driver);
	ath_nand_remove();
}

module_init(ath_nand_init);
module_exit(ath_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_ALIAS("platform:" DRV_NAME);
