/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef RADIX_H
#define RADIX_H

void radix_sort(void *data, void *scratch, size_t nr_members,
		size_t member_size, size_t key_offset);

#endif /* RADIX_H */
