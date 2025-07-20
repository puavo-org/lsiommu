/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "down.h"
#include "iommu.h"
#include "strbuf.h"
#include "sysfs.h"

static int iommu_read_pci_props(const char *dev_path, struct pci_dev_props *props)
{
	const char *bdf;

	down(down_strbuf) struct strbuf *buf = strbuf_alloc(PATH_MAX);
	if (!buf)
		return -ENOMEM;

	bdf = basename((char *)dev_path);

	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/vendor");
	if (sysfs_read_file((const char *)buf->data, props->vendor,
			    sizeof(props->vendor)) < 0)
		return -errno;
	strbuf_clear(buf);

	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/device");
	if (sysfs_read_file((const char *)buf->data, props->device,
			    sizeof(props->device)) < 0)
		return -errno;
	strbuf_clear(buf);

	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/class");
	if (sysfs_read_file((const char *)buf->data, props->class,
			    sizeof(props->class)) < 0)
		return -errno;
	strbuf_clear(buf);

	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/revision");
	props->has_revision =
		sysfs_read_file((const char *)buf->data, props->revision,
				sizeof(props->revision)) >= 0;

	strncpy(props->bdf, bdf, sizeof(props->bdf) - 1);

	props->valid = true;
	return 0;
}

ssize_t iommu_groups_read(struct iommu_group *groups, size_t groups_size)
{
	const char *pci_devices_path = "/sys/bus/pci/devices";
	size_t groups_cnt = 0;
	struct dirent *entry;
	char target_path[PATH_MAX];
	int ret = 0;

	down(down_dir) DIR *dir = opendir(pci_devices_path);
	if (!dir)
		return -errno;

	down(down_strbuf) struct strbuf *buf = strbuf_alloc(PATH_MAX);
	if (!dir || !buf)
		return -errno;

	while ((entry = readdir(dir)) != NULL) {
		struct pci_device *pci_dev;
		struct iommu_group *target;
		const char *dev_path;
		char *endptr;
		ssize_t len;
		long id;
		size_t i;

		if (entry->d_name[0] == '.')
			continue;

		strbuf_append(buf, pci_devices_path);
		strbuf_append(buf, "/");
		strbuf_append(buf, entry->d_name);
		dev_path = (const char *)buf->data;

		strbuf_append(buf, "/iommu_group");
		if (buf->status & STRBUF_OVERFLOW) {
			strbuf_clear(buf);
			continue;
		}

		len = readlink((const char *)buf->data, target_path,
			     sizeof(target_path) - 1);
		strbuf_clear(buf);
		if (len < 0)
			continue;

		target_path[len] = '\0';

		errno = 0;
		id = strtol(basename(target_path), &endptr, 10);
		if (errno != 0 || *endptr != '\0')
			continue;

		target = NULL;
		for (i = 0; i < groups_cnt; i++) {
			if (groups[i].id == id) {
				target = &groups[i];
				break;
			}
		}

		if (!target) {
			if (groups_cnt >= groups_size) {
				ret = -E2BIG;
				goto out;
			}
			target = &groups[groups_cnt];
			target->id = (int)id;
			target->device_count = 0;
			groups_cnt++;
		}

		if (target->device_count >= IOMMU_MAX_DEVICES_PER_GROUP) {
			ret = -E2BIG;
			goto out;
		}

		pci_dev = &target->devices[target->device_count];
		if (pci_dev_read_addr(entry->d_name, &pci_dev->addr) != 0)
			continue;

		if (iommu_read_pci_props(dev_path, &pci_dev->props) != 0)
			pci_dev->props.valid = false;

		target->device_count++;
	}

	ret = iommu_groups_sort(groups, groups_cnt);
	if (ret != 0)
		goto out;

	return groups_cnt;

out:
	return ret;
}
