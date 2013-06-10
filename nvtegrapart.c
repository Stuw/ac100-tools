/*
 *  fs/partitions/nvtegria.c
 *  Copyright (c) 2010 Gilles Grandou
 *
 *  Nvidia uses for its Tegra2 SOCs a proprietary partition system which is
 *  unfortunately undocumented.
 *
 *  Typically a Tegra2 system embedds an internal Flash memory (MTD or MMC)
 *  The bottom of this memory contains the initial bootstrap code which
 *  implements a communication protocol (typically over usb) which allows a
 *  host system (through a tool called nvflash) to access, read, write and
 *  partition the internal flash.
 *
 *  The partition table format is not publicaly documented, and usually
 *  partition description is passed to kernel through the command line
 *  (with tegrapart= argument whose support is available in nv-tegra tree,
 *  see http://nv-tegra.nvidia.com/ )
 *
 *  Rewriting partition table or even switching to a standard msdos is
 *  theorically possible, but it would mean loosing support from nvflash
 *  and from bootloader, while no real alternative exists yet.
 *
 *  Partition table format has been reverse-engineered from analysis of
 *  an existing partition table as found on Toshiba AC100/Dynabook AZ. All
 *  fields have been guessed and there is no guarantee that it will work
 *  in all situation nor in all other Tegra2 based products.
 *
 *
 *  The standard partitions which can be found on an AC100 are the next
 *  ones:
 *
 *  sector size = 2048 bytes
 *
 *  Id  Name    Start   Size            Comment
 *              sector  sectors
 *
 *  1           0       1024            unreachable (bootstrap ?)
 *  2   BCT     1024    512             Boot Configuration Table
 *  3   PT      1536    256             Partition Table
 *  4   EBT     1792    1024            Boot Loader
 *  5   SOS     2816    2560            Recovery Kernel
 *  6   LNX     5376    4096            System Kernel
 *  7   MBR     9472    512             MBR - msdos partition table
 *                                      for the rest of the disk
 *  8   APP     9984    153600          OS root filesystem
 *  ...
 *
 *  the 1024 first sectors are hidden to the hardware one booted
 *  (so 1024 should be removed from numbers found in the partition
 *  table)
 *
 *  There is no standard magic value which can be used for sure
 *  to say that there is a nvtegra partition table at sector 512.
 *  Hence, as an heuristic, we check if the value which would be
 *  found for the BCT partition entry are valid.
 *
 */
/*
 * Based on kernel module
 */

#include <stdio.h>
#include <stdlib.h>

#define printk printf
#define KERN_INFO


struct nvtegra_partinfo {
	unsigned id;
	char name[4];
	unsigned type;
	unsigned unk1[2];
	char name2[4];
	unsigned unk2[4];
	unsigned start;
	unsigned unk3;
	unsigned size;
	unsigned unk4[7];
};

struct nvtegra_ptable {
	unsigned unknown[18];
	struct nvtegra_partinfo partinfo_bct;
	struct nvtegra_partinfo partinfo[23];
};

struct partinfo {
	int valid;
	char name[4];
	unsigned start;
	unsigned size;
};

int nvtegra_partition(FILE* f)
{
	struct nvtegra_ptable *pt;
	struct nvtegra_partinfo *p;
	unsigned len;
	int res;
	int i;
	char *s;

	pt = (struct nvtegra_ptable*)malloc(2048);
	if (!pt)
		return -1;

	if ((res = fread((char *)pt, 1, 2048, f)) != 2048) {
		free(pt);
		printf("fread failed: %d\n", res);
		return -2;
	}

	/* check if BCT partinfo looks correct */
	p = &pt->partinfo_bct;

	printk(KERN_INFO
			"nvtegrapart: #0 id=%d type=%d [%-4.4s] start=%u size=%u\n",
			p->id, p->type, p->name, p->start, p->size);

	if (p->id != 2)
		return 1;
	if (memcmp(p->name, "BCT\0", 4))
		return 2;
	if (p->type != 18)
		return 3;
	if (memcmp(p->name2, "BCT\0", 4))
		return 4;
	if (p->start != 0)
		return 5;
	if (p->size != 1536)
		return 6;

	p = pt->partinfo;

	for (i = 1; (p->id < 128) && (i <= 23); i++) {
			printk(KERN_INFO
				"nvtegrapart: #%d id=%d [%-4.4s] start=%u size=%u\n",
				i, p->id, p->name, p->start * 4 - 0x1000, p->size * 4);
			++p;
	}

	printk("\n");

	free(pt);

	return 0;
}


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <bct.img>\n", argv[0]);
		return 1;
	}

	const char* szFile = argv[1];

	FILE* f = fopen(szFile, "r");
	if (!f)
	{
		printf("Can't open file %s\n", szFile);
		return 2;
	}

	int res = nvtegra_partition(f);

	fclose(f);

	return res;
}
