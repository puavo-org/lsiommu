/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <argtable2.h>
#include <cJSON.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "down.h"
#include "iommu.h"
#include "pci.h"
#include "strbuf.h"

#define _QUOTE(str) #str
#define QUOTE(str) _QUOTE(str)

static const size_t IOMMU_NR_GROUPS = 256;

static int print_plain(struct iommu_group *groups, size_t nr_groups)
{
	size_t i, j;

	down(down_strbuf) struct strbuf *buf = strbuf_alloc(512);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < nr_groups; i++) {
		for (j = 0; j < groups[i].device_count; j++) {
			struct pci_device *dev = &groups[i].devices[j];
			char addr_str[32];

			if (dev->valid) {
				pci_to_strbuf(dev, buf);
				printf("Group %03d %s\n", groups[i].id,
				       (char *)buf->data);
			} else {
				pci_addr_to_string(dev->addr, addr_str,
						       sizeof(addr_str));
				printf("Group %03d %s N/A\n", groups[i].id,
				       addr_str);
			}

			strbuf_clear(buf);
		}
	}

	return 0;
}

static int print_json(struct iommu_group *groups, size_t nr_groups)
{
	cJSON *root = cJSON_CreateObject();
	cJSON *groups_array = NULL;
	char *json_string = NULL;
	size_t i, j;

	if (!root)
		return -ENOMEM;

	groups_array = cJSON_AddArrayToObject(root, "iommu_groups");

	for (i = 0; i < nr_groups; i++) {
		cJSON *group_obj = cJSON_CreateObject();
		cJSON *devices_array = NULL;

		cJSON_AddNumberToObject(group_obj, "id", groups[i].id);
		devices_array = cJSON_AddArrayToObject(group_obj, "devices");

		for (j = 0; j < groups[i].device_count; j++) {
			struct pci_device *dev = &groups[i].devices[j];
			cJSON *device_obj = cJSON_CreateObject();
			char addr_str[32];

			pci_addr_to_string(dev->addr, addr_str,
					       sizeof(addr_str));

			cJSON_AddStringToObject(device_obj, "address",
						addr_str);

			if (dev->valid) {
				cJSON_AddStringToObject(
					device_obj, "class",
					dev->class + 2);
				cJSON_AddStringToObject(
					device_obj, "vendor",
					dev->vendor + 2);
				cJSON_AddStringToObject(
					device_obj, "device",
					dev->device + 2);
				if (dev->has_revision) {
					cJSON_AddStringToObject(
						device_obj, "revision",
						dev->revision + 2);
				}
			}
			cJSON_AddItemToArray(devices_array, device_obj);
		}
		cJSON_AddItemToArray(groups_array, group_obj);
	}

	json_string = cJSON_Print(root);
	if (json_string) {
		printf("%s\n", json_string);
		free(json_string);
	}

	cJSON_Delete(root);
	return 0;
}

int main(int argc, char **argv)
{
	struct arg_lit *help = arg_lit0("h", "help", "Print help and exit");
	struct arg_str *style = arg_str0(NULL, "style", "<style>",
					 "Output style (plain|json)");
	const char *process_name = argv[0];
	struct arg_end *end = arg_end(20);
	void *argtable[] = {help, style, end};
	ssize_t nr_groups = 0;
	bool has_arg_errors;
	(void)argc;
	int ret;

	has_arg_errors = arg_parse(argc, argv, argtable) > 0;

	if (style->count == 0)
		style->sval[0] = "plain";

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
		goto err;
	}

	if (strcmp(style->sval[0], "plain") &&
	    strcmp(style->sval[0], "json")) {
		fprintf(stderr, "error: invalid style '%s'\n",
			style->sval[0]);
		goto err;
	}

	down(down_malloc) struct iommu_group *groups =
		calloc(IOMMU_NR_GROUPS, sizeof(struct iommu_group));
	if (!groups) {
		fprintf(stderr, "no memory\n");
		goto err;
	}

	nr_groups = iommu_groups_read(groups, IOMMU_NR_GROUPS);
	if (nr_groups < 0) {
		fprintf(stderr, "iommu read error: %s", strerror(-nr_groups));
		goto err;
	}

	if (strcmp(style->sval[0], "json") == 0)
		ret = print_json(groups, nr_groups);
	else
		ret = print_plain(groups, nr_groups);

	if (ret) {
		fprintf(stderr, "print error: %s\n", strerror(ret));
		goto err;
	}

out:
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;

err:
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	fprintf(stderr, "Try '%s --help' for more information.\n", process_name);
	return 1;
}
