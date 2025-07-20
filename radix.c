/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 *
 * Based on https://www.cubic.org/docs/radix.htm.
 */

#include <stdint.h>
#include <string.h>
#include "radix.h"

void radix_sort(uint32_t **data, uint32_t **scratch, size_t len)
{
	size_t idx[256];
	size_t b, i, j;
	uint8_t byte;
	void *tmp;

	for (b = 0; b < sizeof(uint32_t); b++) {
		size_t cnt[256] = {0};

		for (i = 0; i < len; i++) {
			byte = (*data[i] >> (b * 8)) & 0xFF;
			cnt[byte]++;
		}

		idx[0] = 0;
		for (i = 1; i < 256; i++)
			idx[i] = idx[i - 1] + cnt[i - 1];

		for (i = 0; i < len; i++) {
			byte = (*data[i] >> (b * 8)) & 0xFF;
			j = idx[byte]++;
			scratch[j] = data[i];
		}

		tmp = data;
		data = scratch;
		scratch = tmp;
	}
}
