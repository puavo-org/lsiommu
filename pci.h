/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef PCI_H
#define PCI_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define PCI_PROPERTY_SIZE 16

struct string_buffer;

struct pci_device {
	uint32_t addr;
	bool valid;
	char class[PCI_PROPERTY_SIZE];
	char vendor[PCI_PROPERTY_SIZE];
	char device[PCI_PROPERTY_SIZE];
	char revision[PCI_PROPERTY_SIZE];
	bool has_revision;
};

int pci_string_to_addr(const char *sysname, uint32_t *addr);
void pci_addr_to_string(uint32_t addr, char *out, size_t size);
void string_buffer_to_pci(struct string_buffer *out, struct pci_device *dev);

#endif /* PCI_H */
