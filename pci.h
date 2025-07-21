/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef PCI_H
#define PCI_H

#include <stdbool.h>
#include <stdint.h>

#define PCI_PROP_SIZE 16

struct strbuf;

struct pci_dev {
	uint32_t addr;
	bool valid;
	char class[PCI_PROP_SIZE];
	char vendor[PCI_PROP_SIZE];
	char device[PCI_PROP_SIZE];
	char revision[PCI_PROP_SIZE];
	bool has_revision;
};

int pci_dev_string_to_addr(const char *sysname, uint32_t *addr);
void pci_dev_addr_to_string(uint32_t addr, char *out, size_t size);
void pci_dev_to_strbuf(struct pci_dev *dev, struct strbuf *out);

#endif /* PCI_H */
