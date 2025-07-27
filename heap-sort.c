/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <alloca.h>
#include <string.h>
#include "heap-sort.h"

static void sift_down(void *base, size_t start, size_t end, size_t size,
		      heap_sort_cmp_t cmp)
{
	void *root_ptr, *swap_ptr, *child_ptr;
	size_t root = start;
	size_t child;
	size_t swap;
	void *temp;

	temp = alloca(size);

	while ((root * 2) + 1 <= end) {
		child = (root * 2) + 1;
		swap = root;

		root_ptr = (char *)base + root * size;
		swap_ptr = (char *)base + swap * size;
		child_ptr = (char *)base + child * size;

		if (cmp(swap_ptr, child_ptr) < 0)
			swap = child;

		if (child + 1 <= end) {
			child_ptr = (char *)base + (child + 1) * size;
			swap_ptr = (char *)base + swap * size;

			if (cmp(swap_ptr, child_ptr) < 0)
				swap = child + 1;
		}

		if (swap == root)
			return;

		swap_ptr = (char *)base + swap * size;

		memcpy(temp, root_ptr, size);
		memcpy(root_ptr, swap_ptr, size);
		memcpy(swap_ptr, temp, size);

		root = swap;
	}
}

void heap_sort(void *base, size_t len, size_t size, heap_sort_cmp_t cmp)
{
	long start;
	size_t end;
	void *temp;

	if (len < 2)
		return;

	temp = alloca(size);

	start = (len / 2) - 1;
	while (start >= 0) {
		sift_down(base, start, len - 1, size, cmp);
		start--;
	}

	end = len - 1;
	while (end > 0) {
		void *start_ptr = (char *)base;
		void *end_ptr = (char *)base + end * size;

		memcpy(temp, end_ptr, size);
		memcpy(end_ptr, start_ptr, size);
		memcpy(start_ptr, temp, size);

		end--;
		sift_down(base, 0, end, size, cmp);
	}
}
