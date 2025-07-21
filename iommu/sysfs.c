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
#include "pci.h"
#include "strbuf.h"
#include "sysfs.h"

static int iommu_read_pci_dev(const char *dev_path, struct pci_dev *pci_dev)
{
	const char *bdf;
	ssize_t ret;

	pci_dev->valid = false;

	bdf = basename((char *)dev_path);

	ret = pci_dev_read_addr(bdf, &pci_dev->addr);
	if (ret)
		return ret;

	down(down_strbuf) struct strbuf *buf = strbuf_alloc(PATH_MAX);
	if (!buf)
		return -ENOMEM;

	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/vendor");
	ret = sysfs_read_file((const char *)buf->data, pci_dev->vendor,
			      sizeof(pci_dev->vendor));
	if (ret < 0)
		return 0;

	strbuf_clear(buf);
	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/device");
	ret = sysfs_read_file((const char *)buf->data, pci_dev->device,
			      sizeof(pci_dev->device));
	if (ret < 0)
		return 0;

	strbuf_clear(buf);
	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/class");
	ret = sysfs_read_file((const char *)buf->data, pci_dev->class,
			      sizeof(pci_dev->class));
	if (ret < 0)
		return 0;

	strbuf_clear(buf);
	strbuf_append(buf, dev_path);
	strbuf_append(buf, "/revision");
	pci_dev->has_revision =
		sysfs_read_file((const char *)buf->data, pci_dev->revision,
				sizeof(pci_dev->revision)) >= 0;

	pci_dev->valid = true;
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
		struct pci_dev *pci_dev;
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

		strbuf_clear(buf);
		strbuf_append(buf, pci_devices_path);
		strbuf_append(buf, "/");
		strbuf_append(buf, entry->d_name);

		if (buf->status & STRBUF_OVERFLOW)
			return -ENOMEM;

		ret = iommu_read_pci_dev((const char *)buf->data, pci_dev);
		if (ret < 0)
			continue;

		target->device_count++;
	}

	ret = iommu_groups_sort(groups, groups_cnt);
	if (ret)
		return ret;

	return groups_cnt;
}
