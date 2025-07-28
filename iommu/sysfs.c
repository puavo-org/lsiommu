/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iommu.h"
#include "pci.h"
#include "string-buffer.h"

#define SYSFS_PCI_DEVICES "/sys/bus/pci/devices"

static ssize_t sysfs_read_file(const char *path, char *buf, size_t size)
{
	int fd = open(path, O_RDONLY);
	ssize_t len;
	int errno_tmp;

	if (fd < 0)
		return -errno;

	len = read(fd, buf, size - 1);
	errno_tmp = errno;
	close(fd);

	if (len < 0)
		return -errno_tmp;

	buf[len] = '\0';

	if (len > 0 && buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
		len--;
	}

	return len;
}

static int iommu_read_pci_device(const char *dev_path, struct pci_device *dev)
{
	STRING_BUFFER(buf, PATH_MAX);
	const char *bdf;
	ssize_t ret;

	dev->valid = false;

	bdf = strrchr(dev_path, '/');
	if (bdf)
		bdf++;
	else
		bdf = dev_path;

	ret = pci_string_to_addr(bdf, &dev->addr);
	if (ret)
		return ret;

	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/vendor");
	ret = sysfs_read_file((const char *)buf->data, dev->vendor,
			      sizeof(dev->vendor));
	if (ret < 0)
		return ret;

	string_buffer_clear(buf);
	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/device");
	ret = sysfs_read_file((const char *)buf->data, dev->device,
			      sizeof(dev->device));
	if (ret < 0)
		return ret;

	string_buffer_clear(buf);
	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/class");
	ret = sysfs_read_file((const char *)buf->data, dev->class,
			      sizeof(dev->class));
	if (ret < 0)
		return ret;

	string_buffer_clear(buf);
	string_buffer_append(buf, dev_path);
	string_buffer_append(buf, "/revision");
	dev->has_revision =
		sysfs_read_file((const char *)buf->data, dev->revision,
				sizeof(dev->revision)) >= 0;

	dev->valid = true;
	return 0;
}

bool iommu_groups_read(struct iommu_group *groups, unsigned int *cnt,
		       unsigned int capacity)
{
	STRING_BUFFER(buf, PATH_MAX);
	struct iommu_group *target;
	struct pci_device *pci_dev;
	char target_path[PATH_MAX];
	struct dirent *entry;
	unsigned int i;
	char *endptr;
	ssize_t len;
	DIR *dir;
	long id;

	*cnt = 0;

	dir = opendir(SYSFS_PCI_DEVICES);
	if (!dir)
		return false;

	errno = 0;
	for (;;) {
		entry = readdir(dir);
		if (!entry)
			break;

		if (entry->d_name[0] == '.')
			continue;

		string_buffer_clear(buf);
		string_buffer_append(buf, SYSFS_PCI_DEVICES);
		string_buffer_append(buf, "/");
		string_buffer_append(buf, entry->d_name);
		string_buffer_append(buf, "/iommu_group");

		if (buf->status & STRING_BUFFER_OVERFLOW)
			continue;

		len = readlink((const char *)buf->data, target_path,
			       sizeof(target_path) - 1);
		if (len < 0)
			continue;

		target_path[len] = '\0';

		errno = 0;
		id = strtol(basename(target_path), &endptr, 10);
		if (errno != 0 || *endptr != '\0' || id < 0)
			continue;

		target = NULL;
		for (i = 0; i < *cnt; i++) {
			if (groups[i].group_id == id) {
				target = &groups[i];
				break;
			}
		}

		if (!target) {
			if (*cnt >= capacity)
				goto err;

			target = &groups[*cnt];
			target->group_id = (unsigned int)id;
			target->nr_devices = 0;
			(*cnt)++;
		}

		if (target->nr_devices >= IOMMU_GROUP_NR_DEVICES)
			goto err;

		pci_dev = &target->devices[target->nr_devices];

		string_buffer_clear(buf);
		string_buffer_append(buf, SYSFS_PCI_DEVICES);
		string_buffer_append(buf, "/");
		string_buffer_append(buf, entry->d_name);

		if (buf->status & STRING_BUFFER_OVERFLOW)
			continue;

		if (iommu_read_pci_device((const char *)buf->data, pci_dev) < 0)
			continue;

		target->nr_devices++;
	}

	if (errno)
		goto err;

	closedir(dir);
	iommu_groups_sort(groups, *cnt);
	return true;

err:
	closedir(dir);
	return false;
}
