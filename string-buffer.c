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
	size_t free_capacity;
	size_t str_len;

	if (buf->status & STRING_BUFFER_OVERFLOW)
		return;

	str_len = strlen(str);
	free_capacity = buf->capacity - buf->length;

	if (str_len >= free_capacity) {
		memcpy(&buf->data[buf->length], str, free_capacity - 1);
		buf->length = buf->capacity;
		buf->data[buf->capacity - 1] = '\0';
		buf->status |= STRING_BUFFER_OVERFLOW;
	} else {
		memcpy(buf->data + buf->length, str, str_len + 1);
		buf->length += str_len;
	}
}
