/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <stdint.h>
#include <string.h>
#include "radix.h"

void radix_sort(void *data, void *scratch, size_t nr_members,
		size_t member_size, size_t key_offset)
{
	uint8_t *scratch_ptr = scratch;
	uint8_t *data_ptr = data;
	size_t cnt[256];
	size_t idx[256];
	size_t b, i, j;
	uint32_t value;
	void *tmp;

	for (b = 0; b < sizeof(uint32_t); b++) {
		memset(cnt, 0, sizeof(cnt));

		for (i = 0; i < nr_members; i++) {
			value = *(uint32_t *)&data_ptr[i * member_size + key_offset];
			cnt[(value >> (b * 8)) & 0xFF]++;
		}

		idx[0] = 0;
		for (i = 1; i < 256; i++)
			idx[i] = idx[i - 1] + cnt[i - 1];

		for (i = 0; i < nr_members; i++) {
			value = *(uint32_t *)&data_ptr[i * member_size + key_offset];
			j = idx[(value >> (b * 8)) & 0xFF]++;
			memcpy(&scratch_ptr[j * member_size], &data_ptr[i * member_size],
			       member_size);
		}

		tmp = data;
		data = scratch;
		scratch = tmp;
	}
}
