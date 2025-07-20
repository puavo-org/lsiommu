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

struct pci_dev_props {
	bool valid;
	char bdf[32];
	char class[PCI_PROP_SIZE];
	char vendor[PCI_PROP_SIZE];
	char device[PCI_PROP_SIZE];
	char revision[PCI_PROP_SIZE];
	bool has_revision;
};

struct pci_device {
	uint32_t addr;
	struct pci_dev_props props;
};

int pci_dev_read_addr(const char *sysname, uint32_t *addr);
void pci_dev_read_prop_string(struct pci_device *pci_dev, struct strbuf *out);

#endif
