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
	size_t i;

	for (i = 0; i < groups_cnt; i++) {
		if (groups[i].device_count > 1) {
			down(down_malloc) struct pci_device *scratch = malloc(
				groups[i].device_count * sizeof(*scratch));
			if (!scratch)
				return -ENOMEM;

			/* Sort PCI devices. */
			radix_sort(groups[i].devices, scratch,
				   groups[i].device_count,
				   sizeof(struct pci_device),
				   offsetof(struct pci_device, addr));
		}
	}

	if (groups_cnt > 1) {
		down(down_malloc) struct iommu_group *scratch =
			malloc(groups_cnt * sizeof(struct iommu_group));
		if (!scratch)
			return -ENOMEM;

		/* Sort IOMMU groups. */
		radix_sort(groups, scratch, groups_cnt,
			   sizeof(struct iommu_group),
			   offsetof(struct iommu_group, id));
	}

	return 0;
}
