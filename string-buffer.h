/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stdalign.h>
#include <stdint.h>

enum string_buffer_flag {
	STRING_BUFFER_OVERFLOW = 0x01,
};

struct string_buffer {
	uint16_t status;
	uint16_t capacity;
	uint16_t length;
	uint8_t data[];
};

#define STRING_BUFFER(name, stack_size)					\
	alignas(struct string_buffer)					\
	uint8_t _stack_buf_##name[stack_size];				\
	struct string_buffer *name =					\
		(struct string_buffer *)_stack_buf_##name;		\
	do {								\
		(name)->status = 0;					\
		(name)->length = 0;					\
		(name)->capacity = (stack_size) - sizeof(*name);	\
		if ((name)->capacity > 0)				\
			(name)->data[0] = '\0';				\
	} while (0)

void string_buffer_clear(struct string_buffer *buf);
void string_buffer_append(struct string_buffer *buf, const char *str);

#endif /* STRING_BUFFER_H */
