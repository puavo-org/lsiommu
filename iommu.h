/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef IOMMU_H
#define IOMMU_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "pci.h"
#include "string-buffer.h"

#define IOMMU_GROUP_NR_DEVICES 32

struct iommu_group {
	unsigned int group_id;
	unsigned int nr_devices;
	struct pci_device devices[IOMMU_GROUP_NR_DEVICES];
};

bool iommu_groups_read(struct iommu_group *groups, unsigned int *cnt,
		       unsigned int capacity);
void iommu_groups_sort(struct iommu_group *groups, unsigned int nr_groups);
const struct string_buffer *iommu_to_json(struct iommu_group *groups,
					  unsigned int nr_groups);

#endif /* IOMMU_H */
