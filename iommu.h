/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef IOMMU_H
#define IOMMU_H

#include <stddef.h>
#include <sys/types.h>

#include "pci.h"

#define IOMMU_GROUP_NR_DEVICES 32

struct iommu_group {
	int id;
	struct pci_device devices[IOMMU_GROUP_NR_DEVICES];
	size_t device_count;
};

ssize_t iommu_groups_read(struct iommu_group *groups, size_t groups_size);
void iommu_groups_free(struct iommu_group *groups, size_t group_count);
int iommu_groups_sort(struct iommu_group *groups, size_t groups_cnt);

#endif /* IOMMU_H */
