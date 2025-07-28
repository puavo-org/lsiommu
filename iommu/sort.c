/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "heap-sort.h"
#include "iommu.h"

static int pci_device_cmp(const void *a, const void *b)
{
	const struct pci_device *dev_a = a;
	const struct pci_device *dev_b = b;

	/* < */
	if (dev_a->addr < dev_b->addr)
		return -1;
	/* > */
	if (dev_a->addr > dev_b->addr)
		return 1;
	/* == */
	return 0;
}

static int iommu_group_cmp(const void *a, const void *b)
{
	const struct iommu_group *group_a = a;
	const struct iommu_group *group_b = b;

	/* < */
	if (group_a->id < group_b->id)
		return -1;
	/* > */
	if (group_a->id > group_b->id)
		return 1;
	/* == */
	return 0;
}

void iommu_groups_sort(struct iommu_group *groups, size_t groups_cnt)
{
	size_t i;

	/* PCI devices */
	for (i = 0; i < groups_cnt; i++)
		if (groups[i].device_count > 1)
			heap_sort(groups[i].devices, groups[i].device_count,
				  sizeof(struct pci_device), pci_device_cmp);

	/* IOMMU groups */
	if (groups_cnt > 1)
		heap_sort(groups, groups_cnt, sizeof(struct iommu_group),
			  iommu_group_cmp);
}
