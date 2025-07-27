/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <stdlib.h>
#include <string.h>

#include "strbuf.h"

struct strbuf *strbuf_alloc(uint16_t size)
{
	struct strbuf *buf;

	if (size <= sizeof(struct strbuf))
		return NULL;

	buf = malloc(size);
	if (!buf)
		return NULL;

	buf->capacity = size - sizeof(struct strbuf);
	strbuf_clear(buf);
	return buf;
}

void strbuf_clear(struct strbuf *buf)
{
	if (!buf)
		return;

	buf->status = 0;
	buf->length = 0;
	if (buf->capacity > 0)
		buf->data[0] = '\0';
}

void strbuf_append(struct strbuf *buf, const char *str)
{
	size_t capacity = buf->capacity;
	size_t free_capacity;
	size_t str_len;

	if (buf->status & STRBUF_OVERFLOW)
		return;

	str_len = strlen(str);
	free_capacity = capacity - buf->length;

	if (str_len >= free_capacity) {
		memcpy(&buf->data[buf->length], str, free_capacity - 1);

		buf->length = capacity;
		buf->data[capacity - 1] = '\0';
		buf->status |= STRBUF_OVERFLOW;
	} else {
		memcpy(buf->data + buf->length, str, str_len + 1);
		buf->length += str_len;
	}
}
