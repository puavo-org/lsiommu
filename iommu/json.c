/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iommu.h"
#include "pci.h"
#include "strbuf.h"

#define IOMMU_JSON_MIN_SIZE	4096
#define IOMMU_JSON_MAX_SIZE	65536

static void iommu_json_append_attribute(struct strbuf *buf, const char *key,
					const char *value)
{
	size_t i;
	char c;

	strbuf_append(buf, "\"");
	for (i = 0; key[i]; i++) {
		c = key[i];
		if ((unsigned char)c < 0x20 || c == '"' || c == '\\') {
			char tmp[7];
			snprintf(tmp, sizeof(tmp), "\\u%04x", (unsigned char)c);
			strbuf_append(buf, tmp);
		} else {
			strbuf_append(buf, (char[]){c, 0});
		}
	}
	strbuf_append(buf, "\":\"");
	for (i = 0; value[i]; i++) {
		c = value[i];
		if ((unsigned char)c < 0x20 || c == '"' || c == '\\') {
			char tmp[7];
			snprintf(tmp, sizeof(tmp), "\\u%04x", (unsigned char)c);
			strbuf_append(buf, tmp);
		} else {
			strbuf_append(buf, (char[]){c, 0});
		}
	}
	strbuf_append(buf, "\"");
}

static void iommu_json_append_pci(struct strbuf *buf, struct pci_device *dev)
{
	strbuf_append(buf, ",");
	iommu_json_append_attribute(buf, "class", dev->class + 2);
	strbuf_append(buf, ",");
	iommu_json_append_attribute(buf, "vendor", dev->vendor + 2);
	strbuf_append(buf, ",");
	iommu_json_append_attribute(buf, "device", dev->device + 2);
	if (dev->has_revision) {
		strbuf_append(buf, ",");
		iommu_json_append_attribute(buf, "revision", dev->revision + 2);
	}
}

static struct strbuf *iommu_try_to_json(struct iommu_group *groups,
				      size_t nr_groups,
				      size_t size)
{
	struct iommu_group *group;
	struct pci_device *dev;
	struct strbuf *buf;
	char tmp[32];
	size_t i, j;

	buf = strbuf_alloc(size);
	if (!buf)
		return NULL;

	strbuf_append(buf, "{\"iommu_groups\":[");

	for (i = 0; i < nr_groups; i++) {
		group = &groups[i];

		if (i > 0)
			strbuf_append(buf, ",");

		strbuf_append(buf, "{\"id\":");
		snprintf(tmp, sizeof(tmp), "%d", group->id);
		strbuf_append(buf, tmp);
		strbuf_append(buf, ",\"devices\":[");

		for (j = 0; j < group->device_count; j++) {
			dev = &group->devices[j];

			if (j > 0)
				strbuf_append(buf, ",");

			strbuf_append(buf, "{");

			pci_addr_to_string(dev->addr, tmp, sizeof(tmp));
			iommu_json_append_attribute(buf, "address", tmp);

			if (dev->valid)
				iommu_json_append_pci(buf, dev);

			strbuf_append(buf, "}");
		}

		strbuf_append(buf, "]");
		strbuf_append(buf, "}");
	}

	strbuf_append(buf, "]}");
	return buf;
}

struct strbuf *iommu_to_json(struct iommu_group *groups, size_t nr_groups)
{
	size_t size = IOMMU_JSON_MIN_SIZE;
	struct strbuf *buf;

	while (size <= IOMMU_JSON_MAX_SIZE) {
		buf = iommu_try_to_json(groups, nr_groups, size);
		if (!buf)
			return NULL;

		if (!(buf->status & STRBUF_OVERFLOW))
			return buf;

		free(buf);
		size <<= 1;
	}

	return NULL;
}
