/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright(c) Opinsys Oy 2025
 */

#ifndef STRBUF_H
#define STRBUF_H

#include <stdint.h>

enum strbuf_status {
	/*
	 * Set when surpassing the buffer capacity, and after that the buffer
	 * is read-only. Caller can postpone checking this bit as the last
	 * step after appending all the required strings.
	 */
	STRBUF_OVERFLOW	= 0x01,

	/*
	 * Set after reading out of boundary.
	 */
	STRBUF_BOUNDARY	= 0x02,
};

struct strbuf {
	uint16_t status;
	uint16_t capacity;
	uint32_t length;
	uint8_t data[];
};

struct strbuf *strbuf_alloc(uint32_t size);
void strbuf_clear(struct strbuf *buf);
void strbuf_append(struct strbuf *buf, const char *str);

#endif /* STRBUF_H */
