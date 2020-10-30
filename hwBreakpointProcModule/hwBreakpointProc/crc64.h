#ifndef CRC64_H_
#define CRC64_H_
#ifdef printk
#include <linux/types.h>
#endif
#define POLY64REV_2     0x95AC9329B7431934ULL
#define INITIALCRC_2    0xFFFFFFFFFFFFFFFFULL
static inline uint64_t my_crc64(char *seq, uint64_t size)
{
	int i, j;
	//int low, high;
	uint64_t crc = INITIALCRC_2, part;
	static int init = 0;
	static uint64_t CRCTable[256];
	uint64_t s = 0;

	if (!init)
	{
		init = 1;
		for (i = 0; i < 256; i++)
		{
			part = i;
			for (j = 0; j < 8; j++)
			{
				if (part & 1)
					part = (part >> 1) ^ POLY64REV_2;
				else
					part >>= 1;
			}
			CRCTable[i] = part;
		}
	}

	for (s = 0; s < size; s++)
	{
		crc = CRCTable[(crc ^ seq[s]) & 0xff] ^ (crc >> 8);
	}

	/*
	 The output is done in two parts to avoid problems with
	 architecture-dependent word order
	 */
	 //low = crc & 0xffffffff;
	 //high = (crc >> 32) & 0xffffffff;
	 //sprintf(res, "%08X%08X", high, low);
	return crc;
}


#endif  /* CRC64_H_ */