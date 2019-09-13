/*
 * Analog Devices bf609 spi driver
 *
 * Copyright (c) 2014 Analog Devices Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <spi.h>
#include <asm/portmux.h>

#include "adi_spi3.h"

#define SPI_PINS(n) \
	{ 0, P_SPI##n##_SCK, P_SPI##n##_MISO, P_SPI##n##_MOSI, 0 }
static unsigned short pins[][5] = {
	[0] = SPI_PINS(0),
	[1] = SPI_PINS(1),
};

#define SPI_CS_PINS(n) \
	{ \
		P_SPI##n##_SSEL1, P_SPI##n##_SSEL2, P_SPI##n##_SSEL3, \
		P_SPI##n##_SSEL4, P_SPI##n##_SSEL5, P_SPI##n##_SSEL6, \
		P_SPI##n##_SSEL7, \
	}
static const unsigned short cs_pins[][7] = {
	[0] = SPI_CS_PINS(0),
	[1] = SPI_CS_PINS(1),
};

int adi_spi_cs_valid(unsigned int bus, unsigned int cs)
{
	if (bus > 1)
		return 0;
	return cs >= 1 && cs <= MAX_CTRL_CS;
}

struct adi_spi_slave *adi_spi_setup(unsigned int bus, unsigned int cs)
{
	struct adi_spi_slave *sdev;

	sdev = spi_alloc_slave(struct adi_spi_slave, bus, cs);
	if (sdev) {
		if (bus)
			sdev->regs = (struct spi_regs *)SPI1_REGBASE;
		else
			sdev->regs = (struct spi_regs *)SPI0_REGBASE;
		pins[bus][0] = cs_pins[bus][cs - 1];
		sdev->pins = pins[bus];
	}
	return sdev;
}
