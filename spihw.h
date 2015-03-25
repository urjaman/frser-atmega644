/*
 * This file is part of the frser-atmega644 project.
 *
 * Copyright (C) 2009,2015 Urja Rannikko <urjaman@gmail.com>
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

#ifndef _SPIHW_H_
#define _SPIHW_H_

/* This will give spi_set_speed if needed. A bit illogically maybe, but anyways... */
#include "frser-flashapi.h"

void spi_select(void);
void spi_deselect(void);
uint8_t spi_txrx(const uint8_t c);
void spi_init(void);
void spi_init_cond(void);
uint8_t spi_uninit(void);

#endif
