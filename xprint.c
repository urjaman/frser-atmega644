/*
 * This file is part of the frser-atmega644 project.
 *
 * Copyright (C) 2015 Urja Rannikko <urjaman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "main.h"
#include <stdarg.h>
#include "lib.h"
#include "xprint.h"

#define XP_BUFSZ 64

static uintptr_t xprint_buf[XP_BUFSZ];
static uint8_t xprint_bwoff = 0;
static uint8_t xprint_broff = 0;
static uint8_t xprint_ovflw = 0;

static uint8_t xpbuf_free(void) {
	uint8_t used;
	if (xprint_bwoff >= xprint_broff) {
		used = xprint_bwoff - xprint_broff;
	} else {
		used = (xprint_bwoff + XP_BUFSZ) - xprint_broff;
	}
	if (used >= (XP_BUFSZ-1)) used = XP_BUFSZ-1;
	return (XP_BUFSZ-1) - used;
}

/* The caller checked that it fits. */
static void xpbuf_put(uintptr_t v) {
	xprint_buf[xprint_bwoff++] = v;
	if (xprint_bwoff >= XP_BUFSZ) xprint_bwoff = 0;
}

static uintptr_t xpbuf_get(void) {
	if (xprint_bwoff == xprint_broff) return 0;
	uintptr_t r = xprint_buf[xprint_broff++];
	if (xprint_broff >= XP_BUFSZ) xprint_broff = 0;
	return r;
}



void xprint_put(uint16_t format, ...) {
	va_list ap;
	uint16_t format_store = format;
	uintptr_t storep[sizeof(uint16_t)*2];
	uint8_t strusage[sizeof(uint16_t)*2];
	uint8_t usage = 0;
	va_start(ap,format);
	while (format) {
		enum partype t = format & 0xF;
		switch (t) {
		default:
			va_end(ap);
			return;
		case PT_LIST_END:
			format = 0;
			break;
		case PT_PMS:
			strusage[usage] = 0;
			storep[usage++] = (uintptr_t)va_arg(ap,void*);
			break;
		case PT_STR: {
			char * s = va_arg(ap,char*);
			size_t l = strlen(s);
			if (l>255) l = 255;
			strusage[usage] = (l+1)/2;
			storep[usage++] = (uintptr_t)s;
			}
			break;
		case PT_HB:
		case PT_HW:
		case PT_DW:
		case PT_DH:
		case PT_CB:
		case PT_CW:
			strusage[usage] = 0;
			storep[usage++] = (uintptr_t)va_arg(ap,int);
			break;
		}
		format = format >> 4;
	}
	va_end(ap);
	int total_usage = 1 + usage;
	for (uint8_t i=0;i<usage;i++) total_usage += strusage[i];
	if (total_usage > xpbuf_free()) {
		if (xprint_ovflw < 255) xprint_ovflw++;
		return;
	}
	if ((xprint_ovflw) && (xpbuf_free() >= (total_usage+4))) {
		/* Output a special message ... */
		if (xprint_ovflw < 255) {
			xpbuf_put(XPT3(PT_PMS,PT_DW,PT_PMS));
			xpbuf_put((uintptr_t)PSTR("xprint() overflow! - lost "));
			xpbuf_put(xprint_ovflw);
			xpbuf_put((uintptr_t)PSTR(" messages\r\n"));
		} else {
			xpbuf_put(PT_PMS);
			xpbuf_put((uintptr_t)PSTR("xprint() overflow! - lost >=255 messages\r\n"));
		}
		xprint_ovflw = 0;
	}
	if (usage < 1) { // There would be no output, wtf.
		return;
	}
	xpbuf_put(format_store);
	format = format_store;
	uint8_t idx = 0;
	while (format) {
		enum partype t = format & 0xF;
		switch (t) {
		case PT_LIST_END:
			format = 0;
			break;
		case PT_STR: {
			char *s = (char*)storep[idx];
			xpbuf_put(strusage[idx]);
			idx++;
			while (*s) {
				unsigned char c1,c2;
				c1 = s[0];
				c2 = s[1];
				xpbuf_put((c2 << 8) | c1);
				if (!c2) break;
				s += 2;
			}
			}
			break;
		default:
			xpbuf_put(storep[idx++]);
			break;
		}
		format = format >> 4;
	}
	return;
}


uint8_t xprint_get(char *buf, uint8_t len)
{
	unsigned char tbuf[6];
	len--; // leave space for the 0
	uint8_t off = 0;
	if (xprint_bwoff == xprint_broff) {
		buf[0] = 0;
		return 0;
	}
	uint16_t format = xpbuf_get();
	while (format) {
		enum partype t = format & 0xF;
		switch (t) {
		case PT_LIST_END:
			format = 0;
			break;
		case PT_STR: {
			uintptr_t l = xpbuf_get();
			while (l) {
				uintptr_t c1c2 = xpbuf_get();
				if (off<len) buf[off++] = c1c2&0xFF;
				if ((off<len) && (c1c2>>8)) buf[off++] = c1c2>>8;
				l--;
			}
			}
			break;
		case PT_PMS: {
			unsigned char c;
			PGM_P p = (PGM_P)xpbuf_get();
			do {
				c = pgm_read_byte(p);
				p++;
				if ((c) && (off<len)) buf[off++] = c;
			} while (c);
			}
			break;
		case PT_HB: {
			uint8_t v = xpbuf_get();
			uchar2xstr(tbuf,v);
			if (off<len) buf[off++] = tbuf[0];
			if (off<len) buf[off++] = tbuf[1];
			}
			break;
		case PT_HW: {
			uint16_t v = xpbuf_get();
			uchar2xstr(tbuf,v>>8);
			if (off<len) buf[off++] = tbuf[0];
			if (off<len) buf[off++] = tbuf[1];
			uchar2xstr(tbuf,v&0xFF);
			if (off<len) buf[off++] = tbuf[0];
			if (off<len) buf[off++] = tbuf[1];
			}
			break;
		case PT_DW: {
			uintptr_t v = xpbuf_get();
			uint2str(tbuf,v);
			for (uint8_t i=0;i<6;i++) {
				if (!tbuf[i]) break;
				if (off<len) buf[off++] = tbuf[i];
			}
			}
			break;
		case PT_DH: {
			uintptr_t v = xpbuf_get();
			uint2str(tbuf,v);
			for (uint8_t i=0;i<6;i++) {
				if (!tbuf[i]) break;
				if (off<len) buf[off++] = tbuf[i];
			}
			if (off<len) buf[off++] = ' ';
			if (off<len) buf[off++] = '(';
			uint2xstr(tbuf,v);
			for (uint8_t i=0;i<6;i++) {
				if (!tbuf[i]) break;
				if (off<len) buf[off++] = tbuf[i];
			}
			if (off<len) buf[off++] = 'h';
			if (off<len) buf[off++] = ')';
			if (off<len) buf[off++] = ' ';
			}
			break;
		case PT_CB: {
			unsigned char c = xpbuf_get();
			if (off<len) buf[off++] = c;
			}
			break;
		case PT_CW: {
			uint16_t mc = xpbuf_get();
			if (off<len) buf[off++] = mc&0xFF;
			if (off<len) buf[off++] = mc>>8;
			}
			break;
		}
		format = format >> 4;
	}
	buf[off] = 0;
	return off;
}
