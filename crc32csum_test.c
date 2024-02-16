//编译:
//	for x86:
//		gcc -g3 crc32c_sw_big.c crc32c_sw_little.c crc32c_x86.c crc32csum_test.c -o x

//	for arm:
//		gcc -g3 crc32c-arm64.c crc32c_sw_big.c crc32c_sw_little.c crc32csum_test.c -o x



/*
 * Copyright (C) 2020 Adan Shaw <adan_shaw@qq.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



//平台筛选(支持arm/x86 的32bit/64bit, 本函数是测试函数, 做要是做高速校验和计算)
#if defined(__aarch64__)
#include "crc32c-arm64.c"
#define crc32c crc32c_arm64
#warning "Using crc32c with arm64 accelerations"
#elif defined(__x86_64__)
#include "crc32c_x86.c"
#define crc32c crc32c_x86
#warning "Using crc32c with x86 accelerations"
#elif __BYTE_ORDER == __BIG_ENDIAN
#include "crc32c_sw_big.c"
#define crc32c crc32c_sw_big
#warning "Using software big endian implementation of crc32c (slow)"
#else
#include "crc32c_sw_little.c"
#define crc32c crc32c_sw_little
#warning "Using software little endian implementation of crc32c (slow)"
#endif



int main (int argc, char **argv)
{
	int fd, ret = 0;
	unsigned char buf[4096];
	ssize_t len;
	uint32_t crc;

	// Use stdin by default
	if (!*++argv)
		*--argv = (char *) "-";

	do
	{
		fd = open (strcmp (*argv, "-") ? *argv : "/dev/stdin", O_RDONLY);
		if (fd < 0)
		{
			fprintf (stderr, "Failed to open %s: %s\n", *argv, strerror (errno));
			ret = 1;
			continue;
		}

		crc = 0;
		while (0 < (len = read (fd, buf, 4096)))
			crc = crc32c (crc, buf, len);

		printf ("%x\t%s\n", crc, *argv);

		close (fd);
	}
	while (*++argv);

	if (ret)
		fprintf (stderr, "WARNING: failed to process some files\n");

	return ret;
}
