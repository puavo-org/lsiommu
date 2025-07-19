/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef IOMMU_H
#define IOMMU_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#define PCI_NO_DESCRIPTION "N/A"
#define PCI_DESCRIPTION_SIZE 256
#define IOMMU_MAX_DEVICES_PER_GROUP 32

struct pci_device {
	uint32_t addr;
	char description[PCI_DESCRIPTION_SIZE];
};

struct iommu_group {
	int id;
	struct pci_device devices[IOMMU_MAX_DEVICES_PER_GROUP];
	size_t device_count;
};

ssize_t iommu_groups_read(struct iommu_group *groups, size_t groups_size);
void iommu_groups_free(struct iommu_group *groups, size_t group_count);

#endif /* IOMMU_H */
