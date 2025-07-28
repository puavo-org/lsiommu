/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef HEAPSORT_H
#define HEAPSORT_H

#include <stddef.h>

typedef int (*heap_sort_cmp_t)(const void *a, const void *b);
void heap_sort(void *base, void *scratch, size_t nr_elements,
	       size_t element_size, heap_sort_cmp_t cmp);

#endif /* HEAPSORT_H */
