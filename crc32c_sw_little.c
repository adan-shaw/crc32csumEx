/* crc32c.c -- compute CRC-32C using the Intel crc32 instruction
 * Copyright (C) 2013, 2015 Mark Adler
 * Version 1.3  31 Dec 2015  Mark Adler
 */

/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu
 */

/* Version history:
   1.0  10 Feb 2013  First version
   1.1   1 Aug 2013  Correct comments on why three crc instructions in parallel
   1.2   1 Nov 2015  Add const qualifier to avoid compiler warning
                     Load entire input into memory (test code)
                     Argument gives number of times to repeat (test code)
                     Argument < 0 forces software implementation (test code)
   1.3  31 Dec 2015  Check for Intel architecture using compiler macro
                     Support big-endian processors in software calculation
                     Add header for external use
 */

#include <stdint.h>
#include <stddef.h>

// CRC-32C (iSCSI) polynomial in reversed bit order.
#define POLY (0x82f63b78)

// Construct table for software CRC-32C little-endian calculation.
static uint32_t crc32c_table_little[8][256];

static __attribute__((constructor)) void crc32c_init_sw_little (void)
{
	unsigned n, k;
	uint32_t crc;
	for (n = 0; n < 256; n++)
	{
		crc = n;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc32c_table_little[0][n] = crc;
	}
	for (n = 0; n < 256; n++)
	{
		crc = crc32c_table_little[0][n];
		for (k = 1; k < 8; k++)
		{
			crc = crc32c_table_little[0][crc & 0xff] ^ (crc >> 8);
			crc32c_table_little[k][n] = crc;
		}
	}
}

// Compute a CRC-32C in software assuming a little-endian architecture, constructing the required table if that hasn't already been done
static inline uint32_t crc32c_sw_little (uint32_t crc, void const *buf, size_t len)
{
	unsigned char const *next = buf;
	uint64_t crcw;

	crc = ~crc;
	while (len && ((uintptr_t) next & 7) != 0)
	{
		crc = crc32c_table_little[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
		len--;
	}
	if (len >= 8)
	{
		crcw = crc;
		do
		{
			crcw ^= *(uint64_t const *) next;
			crcw = crc32c_table_little[7][crcw & 0xff] ^ crc32c_table_little[6][(crcw >> 8) & 0xff] ^ crc32c_table_little[5][(crcw >> 16) & 0xff] ^ crc32c_table_little[4][(crcw >> 24) & 0xff] ^ crc32c_table_little[3][(crcw >> 32) & 0xff] ^ crc32c_table_little[2][(crcw >> 40) & 0xff] ^ crc32c_table_little[1][(crcw >> 48) & 0xff] ^ crc32c_table_little[0][crcw >> 56];
			next += 8;
			len -= 8;
		}
		while (len >= 8);
		crc = crcw;
	}
	while (len)
	{
		crc = crc32c_table_little[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
		len--;
	}
	return ~crc;
}
