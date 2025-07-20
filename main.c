/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <argtable2.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "down.h"
#include "iommu.h"
#include "strbuf.h"

#define _QUOTE(str) #str
#define QUOTE(str) _QUOTE(str)

static const size_t IOMMU_NR_GROUPS = 256;

int main(int argc, char **argv)
{
	struct arg_lit *help = arg_lit0("h", "help", "Print help and exit");
	const char *process_name = argv[0];
	struct arg_end *end = arg_end(20);
	void *argtable[] = {help, end};
	ssize_t nr_groups = 0;
	bool has_arg_errors;
	size_t i, j;
	(void)argc;

	has_arg_errors = arg_parse(argc, argv, argtable) > 0;

	if (help->count > 0) {
		printf("Usage: %s", process_name);
		arg_print_syntax(stdout, argtable, "\n");
		printf("Lists IOMMU groups and their associated PCI devices.\n");
		printf("This version was compiled for %s discovery.\n\n",
		       QUOTE(CONFIG_DISCOVERY));
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		goto out;
	}

	if (has_arg_errors) {
		arg_print_errors(stderr, end, process_name);
		fprintf(stderr, "Try '%s --help' for more information.\n",
			process_name);
		goto err;
	}

	down(down_malloc) struct iommu_group *groups =
		calloc(IOMMU_NR_GROUPS, sizeof(struct iommu_group));
	if (!groups) {
		fprintf(stderr, "no memory\n");
		goto err;
	}

	down(down_strbuf) struct strbuf *buf = strbuf_alloc(512);
	if (!buf) {
		fprintf(stderr, "no memory\n");
		goto err;
	}

	nr_groups = iommu_groups_read(groups, IOMMU_NR_GROUPS);
	if (nr_groups < 0) {
		fprintf(stderr, "iommu read error: %s", strerror(-nr_groups));
		goto err;
	}

	for (i = 0; i < (size_t)nr_groups; i++) {
		for (j = 0; j < groups[i].device_count; j++) {
			struct pci_device *pci_dev = &groups[i].devices[j];

			if (pci_dev->props.valid) {
				pci_dev_read_prop_string(pci_dev, buf);
				printf("Group %03d %s\n", groups[i].id,
				       (char *)buf->data);
			} else {
				printf("Group %03d N/A\n", groups[i].id);
			}

			strbuf_clear(buf);
		}
	}

out:
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;

err:
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 1;
}
