/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iommu.h"
#include "pci.h"
#include "string-buffer.h"

#define _QUOTE(str) #str
#define QUOTE(str) _QUOTE(str)

static const size_t IOMMU_NR_GROUPS = 256;

static int print_plain(struct iommu_group *groups, size_t nr_groups)
{
	size_t i, j;
	STRING_BUFFER(buf, 512);

	for (i = 0; i < nr_groups; i++) {
		for (j = 0; j < groups[i].device_count; j++) {
			struct pci_device *dev = &groups[i].devices[j];
			char addr_str[32];

			if (dev->valid) {
				string_buffer_to_pci(buf, dev);
				printf("Group %03d %s\n", groups[i].id,
				       (char *)buf->data);
			} else {
				pci_addr_to_string(dev->addr, addr_str,
						   sizeof(addr_str));
				printf("Group %03d %s N/A\n", groups[i].id,
				       addr_str);
			}

			string_buffer_clear(buf);
		}
	}

	return 0;
}

static int print_json(struct iommu_group *groups, size_t nr_groups)
{
	const struct string_buffer *json_buf = iommu_to_json(groups, nr_groups);

	if (!json_buf)
		return -ENOMEM;

	printf("%s\n", (char *)json_buf->data);
	return 0;
}

static void print_usage(const char *name)
{
	printf("Usage: %s [-h] [--format <format>]\n", name);
	printf("Lists IOMMU groups and their associated PCI devices.\n");
	printf("This version was compiled for %s discovery.\n\n",
	       QUOTE(CONFIG_DISCOVERY));
	printf("  -h, --help            Print help and exit\n");
	printf("      --format <format> Output format (plain|json), default: plain\n");
}

int main(int argc, char **argv)
{
	const char *process_name = argv[0];
	const char *format = "plain";
	struct iommu_group *groups = NULL;
	ssize_t nr_groups = 0;
	int ret;
	int opt;

	static struct option long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "format", required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	for (;;) {
		opt = getopt_long(argc, argv, "hs:", long_options, NULL);
		if (opt == -1)
			break;

		switch (opt) {
		case 'h':
			print_usage(process_name);
			goto out;
		case 's':
			format = optarg;
			break;
		default:
			goto err;
		}
	}

	if (strcmp(format, "plain") && strcmp(format, "json")) {
		fprintf(stderr, "error: invalid format '%s'\n", format);
		goto err;
	}

	groups = calloc(IOMMU_NR_GROUPS, sizeof(struct iommu_group));
	if (!groups) {
		fprintf(stderr, "no memory\n");
		goto err;
	}

	nr_groups = iommu_groups_read(groups, IOMMU_NR_GROUPS);
	if (nr_groups < 0) {
		fprintf(stderr, "iommu read error: %s", strerror(-nr_groups));
		goto err;
	}

	if (strcmp(format, "json") == 0)
		ret = print_json(groups, nr_groups);
	else
		ret = print_plain(groups, nr_groups);

	if (ret) {
		fprintf(stderr, "print error: %s\n", strerror(ret));
		goto err;
	}

out:
	free(groups);
	return 0;

err:
	fprintf(stderr, "Try '%s --help' for more information.\n",
		process_name);
	free(groups);
	return 1;
}
