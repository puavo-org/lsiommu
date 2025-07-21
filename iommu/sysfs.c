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

static int pci_dev_read_sysfs_props(const char *dev_path, struct pci_dev_props *props)
{
	const char *bdf;
	ssize_t ret;

	props->valid = false;

	down(down_strbuf) struct strbuf *buf = strbuf_alloc(PATH_MAX);
	if (!buf)
		return -ENOMEM;

	bdf = basename((char *)dev_path);

	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/vendor");
	ret = sysfs_read_file((const char *)buf->data, props->vendor,
			      sizeof(props->vendor));
	if (ret < 0)
		return ret;

	strbuf_clear(buf);
	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/device");
	ret = sysfs_read_file((const char *)buf->data, props->device,
			      sizeof(props->device));
	if (ret < 0)
		return ret;

	strbuf_clear(buf);
	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/class");
	ret = sysfs_read_file((const char *)buf->data, props->class,
			      sizeof(props->class));
	if (ret < 0)
		return ret;

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
	if (!buf)
		return -errno;

	while ((entry = readdir(dir)) != NULL) {
		struct pci_device *pci_dev;
		struct iommu_group *target;
		char *endptr;
		ssize_t len;
		long id;
		size_t i;

		if (entry->d_name[0] == '.')
			continue;

		strbuf_clear(buf);
		strbuf_append(buf, pci_devices_path);
		strbuf_append(buf, "/");
		strbuf_append(buf, entry->d_name);
		strbuf_append(buf, "/iommu_group");

		if (buf->status & STRBUF_OVERFLOW)
			return -ENOMEM;

		len = readlink((const char *)buf->data, target_path,
			       sizeof(target_path) - 1);
		if (len < 0)
			return len;

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
			if (groups_cnt >= groups_size)
				return -E2BIG;
			target = &groups[groups_cnt];
			target->id = (int)id;
			target->device_count = 0;
			groups_cnt++;
		}

		if (target->device_count >= IOMMU_MAX_DEVICES_PER_GROUP)
			return -E2BIG;

		pci_dev = &target->devices[target->device_count];
		ret = pci_dev_read_addr(entry->d_name, &pci_dev->addr);
		if (ret)
			continue;

		strbuf_clear(buf);
		strbuf_append(buf, pci_devices_path);
		strbuf_append(buf, "/");
		strbuf_append(buf, entry->d_name);

		if (buf->status & STRBUF_OVERFLOW)
			return -ENOMEM;

		ret = pci_dev_read_sysfs_props((const char *)buf->data, &pci_dev->props);
		if (ret)
			pci_dev->props.valid = false;

		target->device_count++;
	}

	ret = iommu_groups_sort(groups, groups_cnt);
	if (ret)
		return ret;

	return groups_cnt;
}
