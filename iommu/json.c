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
#include "string-buffer.h"

#define IOMMU_JSON_BUFFER_SIZE 65536

alignas(struct string_buffer) static uint8_t iommu_json_buffer[IOMMU_JSON_BUFFER_SIZE];

static void iommu_json_append_attribute(struct string_buffer *buf,
					const char *key, const char *value)
{
	size_t i;
	char c;
	char tmp[7];

	string_buffer_append(buf, "\"");
	for (i = 0; key[i]; i++) {
		c = key[i];
		if ((unsigned char)c < 0x20 || c == '"' || c == '\\') {
			snprintf(tmp, sizeof(tmp), "\\u%04x", (unsigned char)c);
			string_buffer_append(buf, tmp);
		} else {
			string_buffer_append(buf, (char[]){ c, 0 });
		}
	}
	string_buffer_append(buf, "\":\"");
	for (i = 0; value[i]; i++) {
		c = value[i];
		if ((unsigned char)c < 0x20 || c == '"' || c == '\\') {
			snprintf(tmp, sizeof(tmp), "\\u%04x", (unsigned char)c);
			string_buffer_append(buf, tmp);
		} else {
			string_buffer_append(buf, (char[]){ c, 0 });
		}
	}
	string_buffer_append(buf, "\"");
}

static void iommu_json_append_pci(struct string_buffer *buf,
				  struct pci_device *dev)
{
	string_buffer_append(buf, ",");
	iommu_json_append_attribute(buf, "class", dev->class + 2);
	string_buffer_append(buf, ",");
	iommu_json_append_attribute(buf, "vendor", dev->vendor + 2);
	string_buffer_append(buf, ",");
	iommu_json_append_attribute(buf, "device", dev->device + 2);
	if (dev->has_revision) {
		string_buffer_append(buf, ",");
		iommu_json_append_attribute(buf, "revision", dev->revision + 2);
	}
}

const struct string_buffer *iommu_to_json(struct iommu_group *groups,
					  unsigned int nr_groups)
{
	struct string_buffer *buf = (struct string_buffer *)iommu_json_buffer;
	struct iommu_group *group;
	struct pci_device *dev;
	unsigned int i, j;
	char tmp[32];

	buf->capacity = IOMMU_JSON_BUFFER_SIZE - sizeof(struct string_buffer);
	string_buffer_clear(buf);

	string_buffer_append(buf, "{\"iommu_groups\":[");

	for (i = 0; i < nr_groups; i++) {
		group = &groups[i];

		if (i > 0)
			string_buffer_append(buf, ",");

		string_buffer_append(buf, "{\"id\":");
		snprintf(tmp, sizeof(tmp), "%u", group->group_id);
		string_buffer_append(buf, tmp);
		string_buffer_append(buf, ",\"devices\":[");

		for (j = 0; j < group->nr_devices; j++) {
			dev = &group->devices[j];

			if (j > 0)
				string_buffer_append(buf, ",");

			string_buffer_append(buf, "{");

			pci_addr_to_string(dev->addr, tmp, sizeof(tmp));
			iommu_json_append_attribute(buf, "address", tmp);

			if (dev->valid)
				iommu_json_append_pci(buf, dev);

			string_buffer_append(buf, "}");
		}

		string_buffer_append(buf, "]}");
	}

	string_buffer_append(buf, "]}");

	if (buf->status & STRING_BUFFER_OVERFLOW)
		return NULL;

	return buf;
}
