/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <string.h>
#include "heap-sort.h"

static void sift_down(void *base, void *scratch, size_t start, size_t end,
		      size_t element_size, heap_sort_cmp_t cmp)
{
	void *root_ptr, *swap_ptr, *child_ptr;
	size_t root = start;
	size_t child;
	size_t swap;

	while ((root * 2) + 1 <= end) {
		child = (root * 2) + 1;
		swap = root;

		root_ptr = (char *)base + root * element_size;
		swap_ptr = (char *)base + swap * element_size;
		child_ptr = (char *)base + child * element_size;

		if (cmp(swap_ptr, child_ptr) < 0)
			swap = child;

		if (child + 1 <= end) {
			child_ptr = (char *)base + (child + 1) * element_size;
			swap_ptr = (char *)base + swap * element_size;

			if (cmp(swap_ptr, child_ptr) < 0)
				swap = child + 1;
		}

		if (swap == root)
			return;

		swap_ptr = (char *)base + swap * element_size;

		memcpy(scratch, root_ptr, element_size);
		memcpy(root_ptr, swap_ptr, element_size);
		memcpy(swap_ptr, scratch, element_size);

		root = swap;
	}
}

void heap_sort(void *base, void *scratch, size_t nr_elements,
	       size_t element_size, heap_sort_cmp_t cmp)
{
	void *start_ptr;
	void *end_ptr;
	long start;
	size_t end;

	if (nr_elements < 2)
		return;

	start = (nr_elements / 2) - 1;
	while (start >= 0) {
		sift_down(base, scratch, start, nr_elements - 1, element_size,
			  cmp);
		start--;
	}

	end = nr_elements - 1;
	while (end > 0) {
		start_ptr = (char *)base;
		end_ptr = (char *)base + end * element_size;

		memcpy(scratch, end_ptr, element_size);
		memcpy(end_ptr, start_ptr, element_size);
		memcpy(start_ptr, scratch, element_size);

		end--;
		sift_down(base, scratch, 0, end, element_size, cmp);
	}
}
