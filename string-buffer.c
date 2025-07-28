/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#include <stdlib.h>
#include <string.h>
#include "string-buffer.h"

void string_buffer_clear(struct string_buffer *buf)
{
	if (!buf)
		return;

	buf->status = 0;
	buf->length = 0;
	if (buf->capacity > 0)
		buf->data[0] = '\0';
}

void string_buffer_append(struct string_buffer *buf, const char *str)
{
	size_t len, rest;

	if (buf->status & STRING_BUFFER_OVERFLOW)
		return;

	len = strlen(str);

	if (buf->length + len < buf->capacity) {
		memcpy(buf->data + buf->length, str, len + 1);
		buf->length += len;
		return;
	}

	buf->status |= STRING_BUFFER_OVERFLOW;

	if (buf->capacity > buf->length + 1)
		rest = buf->capacity - buf->length - 1;
	else
		return;

	memcpy(&buf->data[buf->length], str, rest);
	buf->length += rest;
	buf->data[buf->length] = '\0';
}
