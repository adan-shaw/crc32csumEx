#define CRC32C3X8(ITR) \
	crc1 = __crc32cd(crc1, *((const uint64_t *)data + 42*1 + (ITR)));\
	crc2 = __crc32cd(crc2, *((const uint64_t *)data + 42*2 + (ITR)));\
	crc0 = __crc32cd(crc0, *((const uint64_t *)data + 42*0 + (ITR)));

#define CRC32C7X3X8(ITR) do {\
	CRC32C3X8((ITR)*7+0) \
	CRC32C3X8((ITR)*7+1) \
	CRC32C3X8((ITR)*7+2) \
	CRC32C3X8((ITR)*7+3) \
	CRC32C3X8((ITR)*7+4) \
	CRC32C3X8((ITR)*7+5) \
	CRC32C3X8((ITR)*7+6) \
	} while(0)

#include <stdint.h>
#include <stddef.h>
#include <arm_acle.h>
#include <arm_neon.h>

/*
 * Function to calculate reflected crc with PMULL Instruction
 * crc done "by 3" for fixed input block size of 1024 bytes
 */
static inline uint32_t crc32c_arm64 (uint32_t crc, unsigned char const *data, unsigned long length)
{
	// Load two consts: K1 and K2
	const poly64_t k1 = 0xe417f38a, k2 = 0x8f158014;
	uint64_t t0, t1;

	uint32_t crc0, crc1, crc2;
	signed long len = length;
	crc = ~crc;

	while ((len -= 1024) >= 0)
	{
		// Do first 8 bytes here for better pipelining
		crc0 = __crc32cd (crc, *(const uint64_t *) data);
		crc1 = 0;
		crc2 = 0;
		data += sizeof (uint64_t);

		// Process block inline, Process crc0 last to avoid dependency with above
		CRC32C7X3X8 (0);
		CRC32C7X3X8 (1);
		CRC32C7X3X8 (2);
		CRC32C7X3X8 (3);
		CRC32C7X3X8 (4);
		CRC32C7X3X8 (5);

		data += 42 * 3 * sizeof (uint64_t);

		/* Merge crc0 and crc1 into crc2
		   crc1 multiply by K2
		   crc0 multiply by K1 */

		t1 = (uint64_t) vmull_p64 (crc1, k2);
		t0 = (uint64_t) vmull_p64 (crc0, k1);
		crc = __crc32cd (crc2, *(const uint64_t *) data);
		crc1 = __crc32cd (0, t1);
		crc ^= crc1;
		crc0 = __crc32cd (0, t0);
		crc ^= crc0;

		data += sizeof (uint64_t);
	}

	if (!(len += 1024))
		return ~crc;

	while ((len -= sizeof (uint64_t)) >= 0)
	{
		crc = __crc32cd (crc, *(const uint64_t *) data);
		data += sizeof (uint64_t);
	}

	// The following is more efficient than the straight loop
	if (len & sizeof (uint32_t))
	{
		crc = __crc32cw (crc, *(const uint32_t *) data);
		data += sizeof (uint32_t);
	}
	if (len & sizeof (uint16_t))
	{
		crc = __crc32ch (crc, *(const uint16_t *) data);
		data += sizeof (uint16_t);
	}
	if (len & sizeof (uint8_t))
	{
		crc = __crc32cb (crc, *(const uint8_t *) data);
	}

	return ~crc;
}
