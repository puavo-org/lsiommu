/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <argtable2.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iommu.h"

#define IOMMU_GROUPS_SIZE 256

int main(int argc, char **argv)
{
	struct arg_lit *help = arg_lit0("h", "help", "Print help and exit");
	struct arg_end *end = arg_end(20);
	void *argtable[] = {help, end};
	struct iommu_group groups[IOMMU_GROUPS_SIZE];
	const char *process_name = argv[0];
	ssize_t nr_groups = 0;
	bool arg_has_errors;
	size_t i, j;
	(void)argc;

	nr_groups = iommu_groups_read(groups, IOMMU_GROUPS_SIZE);
	if (nr_groups < 0) {
		fprintf(stderr, "iommu read error: %s", strerror(nr_groups));
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

	for (i = 0; i < (size_t)nr_groups; i++) {
		printf("iommu group %d\n", groups[i].id);
		for (j = 0; j < groups[i].device_count; j++)
			printf("\t%s\n", groups[i].devices[j].description);
	}

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}
