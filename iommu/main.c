/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "down.h"
#include "iommu.h"
#include "radix.h"
#include "strbuf.h"

int iommu_groups_sort(struct iommu_group *groups, size_t groups_cnt)
{
	size_t i, j;

	for (i = 0; i < groups_cnt; i++) {
		if (groups[i].device_count > 1) {
			size_t count = groups[i].device_count;

			down(down_malloc) struct pci_device **dev_ptrs =
				malloc(count * sizeof(*dev_ptrs));
			if (!dev_ptrs)
				return -ENOMEM;

			for (j = 0; j < count; j++)
				dev_ptrs[j] = &groups[i].devices[j];

			down(down_malloc) struct pci_device **scratch =
				malloc(count * sizeof(*scratch));
			if (!scratch)
				return -ENOMEM;

			/* Sort PCI devices. */
			radix_sort((uint32_t **)dev_ptrs, (uint32_t **)scratch, count);

			down(down_malloc) struct pci_device *sorted =
				malloc(count * sizeof(*sorted));
			if (!sorted)
				return -ENOMEM;

			for (j = 0; j < count; j++)
				sorted[j] = *dev_ptrs[j];

			memcpy(groups[i].devices, sorted,
			       count * sizeof(struct pci_device));
		}
	}

	if (groups_cnt > 1) {
		down(down_malloc) struct iommu_group **group_ptrs =
			malloc(groups_cnt * sizeof(*group_ptrs));
		if (!group_ptrs)
			return -ENOMEM;

		for (i = 0; i < groups_cnt; i++)
			group_ptrs[i] = &groups[i];

		down(down_malloc) struct iommu_group **scratch =
			malloc(groups_cnt * sizeof(*scratch));
		if (!scratch)
			return -ENOMEM;

		/* Sort IOMMU groups. */
		radix_sort((uint32_t **)group_ptrs, (uint32_t **)scratch, groups_cnt);

		down(down_malloc) struct iommu_group *sorted =
			malloc(groups_cnt * sizeof(*sorted));
		if (!sorted)
			return -ENOMEM;

		for (i = 0; i < groups_cnt; i++)
			sorted[i] = *group_ptrs[i];

		memcpy(groups, sorted, groups_cnt * sizeof(struct iommu_group));
	}

	return 0;
}
