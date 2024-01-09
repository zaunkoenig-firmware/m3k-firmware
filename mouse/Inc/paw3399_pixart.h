/* MIT License
 *
 * Copyright (c) 2023 Zaunkoenig GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <delay.h>
#include <m3k_resource.h>
#include <stdint.h>
#include "stm32f7xx.h"
#include "config.h"

static void spi_init(void)
{
	// GPIO config
	SPIx_SCK_GPIO_CLK_ENABLE();
	SPIx_MISO_GPIO_CLK_ENABLE();
	SPIx_MOSI_GPIO_CLK_ENABLE();
	SPIx_SS_GPIO_CLK_ENABLE();

	// SCK
	MODIFY_REG(SPIx_SCK_GPIO_PORT->MODER,
			0b11 << (2*SPIx_SCK_PIN_Pos),
			0b10 << (2*SPIx_SCK_PIN_Pos));
	MODIFY_REG(SPIx_SCK_GPIO_PORT->AFR[SPIx_SCK_PIN_Pos >= 8],
			0b1111 << ((4*SPIx_SCK_PIN_Pos)%32),
			SPIx_SCK_AF << ((4*SPIx_SCK_PIN_Pos)%32));
	// MISO
	MODIFY_REG(SPIx_MISO_GPIO_PORT->MODER,
			0b11 << (2*SPIx_MISO_PIN_Pos),
			0b10 << (2*SPIx_MISO_PIN_Pos));
	MODIFY_REG(SPIx_MISO_GPIO_PORT->PUPDR,
			0b11 << (2*SPIx_MISO_PIN_Pos),
			0b10 << (2*SPIx_MISO_PIN_Pos));
	MODIFY_REG(SPIx_MISO_GPIO_PORT->AFR[SPIx_MISO_PIN_Pos >= 8],
			0b1111 << ((4*SPIx_MISO_PIN_Pos)%32),
			SPIx_MISO_AF << ((4*SPIx_MISO_PIN_Pos)%32));
	// MOSI
	MODIFY_REG(SPIx_MOSI_GPIO_PORT->MODER,
			0b11 << (2*SPIx_MOSI_PIN_Pos),
			0b10 << (2*SPIx_MOSI_PIN_Pos));
	MODIFY_REG(SPIx_MOSI_GPIO_PORT->AFR[SPIx_MOSI_PIN_Pos >= 8],
			0b1111 << ((4*SPIx_MOSI_PIN_Pos)%32),
			SPIx_MOSI_AF << ((4*SPIx_MOSI_PIN_Pos)%32));
	// PB6 SS
	MODIFY_REG(SPIx_SS_PORT->MODER,
			0b11 << (2*SPIx_SS_PIN_Pos),
			0b01 << (2*SPIx_SS_PIN_Pos));

	// SPI config
	SPIx_CLK_ENABLE();
	SPIx->CR1 = SPI_CR1_SSM | SPI_CR1_SSI // software SS
			| (0b001 << SPI_CR1_BR_Pos) // assumes PCLK2 = 32MHz. divide by 4 for 8MHz
			| SPI_CR1_MSTR // master
			| SPI_CR1_CPOL // CPOL = 1
			| SPI_CR1_CPHA; // CPHA = 1
	SPIx->CR2 = SPI_CR2_FRXTH // 8-bit level for RXNE
			| (0b0111 << SPI_CR2_DS_Pos); // 8-bit data
	SPIx->CR1 |=  SPI_CR1_SPE; // enable SPI
}

static inline void ss_low(void)
{
	SPIx_SS_PORT->ODR &= ~SPIx_SS_PIN;
}

static inline void ss_high(void)
{
	SPIx_SS_PORT->ODR |= SPIx_SS_PIN;
}

static inline uint8_t spi_sendrecv(uint8_t b)
{
    while (!(SPIx->SR & SPI_SR_TXE));
    *(__IO uint8_t *)&SPIx->DR = b;
    while (!(SPIx->SR & SPI_SR_RXNE));
    return *(__IO uint8_t *)&SPIx->DR;
}

#define spi_recv(x) spi_sendrecv(0)
#define spi_send(x) (void)spi_sendrecv(x)

static void spi_write(const uint8_t addr, const uint8_t data) {
	spi_send(addr | 0x80);
	spi_send(data);
	delay_us(5); // maximum of t_SWW, t_SWR
}

static uint8_t spi_read(const uint8_t addr) {
    spi_send(addr);
    delay_us(2); // t_SRAD
    uint8_t rd = spi_recv();
	delay_us(2); // maximum of t_SRW, t_SRR
	return rd;
}

// 6.2.1-99
static void paw3399_init_reg(void)
{
	ss_low();
	spi_write(0x40, 0x80); // 1
	spi_write(0x7F, 0x0E); // 2
	spi_write(0x55, 0x0D); // 3
	spi_write(0x56, 0x1B); // 4
	spi_write(0x57, 0xE8); // 5
	spi_write(0x58, 0xD5); // 6
	spi_write(0x7F, 0x14); // 7
	spi_write(0x42, 0xBC); // 8
	spi_write(0x43, 0x74); // 9
	spi_write(0x4B, 0x20); // 10
	spi_write(0x4D, 0x00); // 11
	spi_write(0x53, 0x0D); // 12
	spi_write(0x7F, 0x05); // 13
	spi_write(0x51, 0x40); // 14
	spi_write(0x53, 0x40); // 15
	spi_write(0x55, 0xCA); // 16
	spi_write(0x61, 0x31); // 17
	spi_write(0x62, 0x64); // 18
	spi_write(0x6D, 0xB8); // 19
	spi_write(0x6E, 0x0F); // 20
	spi_write(0x70, 0x02); // 21
	spi_write(0x4A, 0x2A); // 22
	spi_write(0x60, 0x26); // 23
	spi_write(0x7F, 0x06); // 24
	spi_write(0x6D, 0x70); // 25
	spi_write(0x6E, 0x60); // 26
	spi_write(0x6F, 0x04); // 27
	spi_write(0x53, 0x02); // 28
	spi_write(0x55, 0x11); // 29
	spi_write(0x7D, 0x51); // 30
	spi_write(0x7F, 0x08); // 31
	spi_write(0x71, 0x4F); // 32
	spi_write(0x7F, 0x09); // 33
	spi_write(0x62, 0x1F); // 34
	spi_write(0x63, 0x1F); // 35
	spi_write(0x65, 0x03); // 36
	spi_write(0x66, 0x03); // 37
	spi_write(0x67, 0x1F); // 38
	spi_write(0x68, 0x1F); // 39
	spi_write(0x69, 0x03); // 40
	spi_write(0x6A, 0x03); // 41
	spi_write(0x6C, 0x1F); // 42
	spi_write(0x6D, 0x1F); // 43
	spi_write(0x51, 0x04); // 44
	spi_write(0x53, 0x20); // 45
	spi_write(0x54, 0x20); // 46
	spi_write(0x71, 0x0F); // 47
	spi_write(0x72, 0x0A); // 48
	spi_write(0x7F, 0x0A); // 49
	spi_write(0x4A, 0x14); // 50
	spi_write(0x4C, 0x14); // 51
	spi_write(0x55, 0x19); // 52
	spi_write(0x7F, 0x14); // 53
	spi_write(0x63, 0x16); // 54
	spi_write(0x7F, 0x0C); // 55
	spi_write(0x41, 0x30); // 56
	spi_write(0x55, 0x14); // 57
	spi_write(0x49, 0x0A); // 58
	spi_write(0x42, 0x00); // 59
	spi_write(0x44, 0x0D); // 60
	spi_write(0x4A, 0x12); // 61
	spi_write(0x4B, 0x09); // 62
	spi_write(0x4C, 0x30); // 63
	spi_write(0x5A, 0x0D); // 64
	spi_write(0x5F, 0x1E); // 65
	spi_write(0x5B, 0x05); // 66
	spi_write(0x5E, 0x0F); // 67
	spi_write(0x7F, 0x0D); // 68
	spi_write(0x48, 0xDD); // 69
	spi_write(0x4F, 0x03); // 70
	spi_write(0x5A, 0x29); // 71
	spi_write(0x5B, 0x47); // 72
	spi_write(0x5C, 0x81); // 73
	spi_write(0x5D, 0x40); // 74
	spi_write(0x71, 0xDC); // 75
	spi_write(0x70, 0x07); // 76
	spi_write(0x73, 0x00); // 77
	spi_write(0x72, 0x08); // 78
	spi_write(0x75, 0xDC); // 79
	spi_write(0x74, 0x07); // 80
	spi_write(0x77, 0x00); // 81
	spi_write(0x76, 0x08); // 82
	spi_write(0x7F, 0x10); // 83
	spi_write(0x4C, 0xD0); // 84
	spi_write(0x7F, 0x00); // 85
	spi_write(0x4F, 0x63); // 86
	spi_write(0x4E, 0x00); // 87
	spi_write(0x52, 0x63); // 88
	spi_write(0x51, 0x00); // 89
	spi_write(0x5A, 0x10); // 90
	spi_write(0x77, 0x4F); // 91
	spi_write(0x47, 0x01); // 92
	spi_write(0x5B, 0x40); // 93
	spi_write(0x66, 0x13); // 94
	spi_write(0x67, 0x0F); // 95
	spi_write(0x78, 0x01); // 96
	spi_write(0x79, 0x9C); // 97
	spi_write(0x55, 0x02); // 98
	spi_write(0x23, 0x70); // 99
	ss_high();
}

static void paw3399_corded_gaming(void)
{
	ss_low();
	spi_write(0x7F, 0x05);
	spi_write(0x51, 0x40);
	spi_write(0x53, 0x40);
	spi_write(0x61, 0x31);
	spi_write(0x6E, 0x0F);
	spi_write(0x7F, 0x07);
	spi_write(0x42, 0x2F);
	spi_write(0x43, 0x00);
	spi_write(0x7F, 0x0D);
	spi_write(0x51, 0x12);
	spi_write(0x52, 0xDB);
	spi_write(0x53, 0x12);
	spi_write(0x54, 0xDC);
	spi_write(0x55, 0x12);
	spi_write(0x56, 0xE4);
	spi_write(0x57, 0x15);
	spi_write(0x58, 0x2D);
	spi_write(0x7F, 0x14);
	spi_write(0x63, 0x1E);
	spi_write(0x7F, 0x00);
	spi_write(0x54, 0x55);
	spi_write(0x40, 0x83);
	ss_high();
}

static void paw3399_set_dpi(const uint16_t dpi)
{
	ss_low();
	spi_write(0x48, dpi & 0xff); // RESOLUTION_X_LOW
	spi_write(0x49, dpi >> 8); // RESOLUTION_X_HIGH
	spi_write(0x4A, dpi & 0xff); // RESOLUTION_Y_LOW
	spi_write(0x4B, dpi >> 8); //RESOLUTION_Y_HIGH
	spi_write(0x47, 0x01); // SET_RESOLUTION
	ss_high();
}

static void paw3399_set_as(const uint8_t angle_snap)
{
	ss_low();
	spi_write(0x56, (angle_snap << 7) | 0x0D);
	ss_high();
}

static void paw3399_set_lod(const uint8_t lod)
{
	ss_low();
	spi_write(0x7F, 0x0C);
	spi_write(0x4E, 0x08 | lod);
	spi_write(0x7F, 0x00);
	ss_high();
}

static void paw3399_init(const Config cfg)
{
	const uint16_t dpi = _FLD2VAL(CONFIG_DPI, cfg);
	const uint8_t ang_snap = (cfg & CONFIG_ANGLE_SNAP_ON) ? 1 : 0;
	const uint8_t lod = _FLD2VAL(CONFIG_LOD, cfg);

	//
	delay_ms(50); // 6.1.2
	ss_high(); // 6.1.3
	delay_ms(3);
	ss_low();
	delay_ms(3);
	ss_high();
	delay_ms(3);

	ss_low();
	spi_write(0x3A, 0x5A); // 6.1.4
	ss_high();
	delay_ms(5); // 6.1.5

	// 6.1.6
	paw3399_init_reg();
	// 6.2.100-107
	ss_low();
	spi_write(0x22, 0x01);
	ss_high();
	int i;
	for (i = 0; i < 60; i++) {
		ss_high();
		delay_us(992); // TODO tune this and/or use dedicated timer or systick
		ss_low();
		if (spi_read(0x6C) == 0x80) break;
	}
	ss_low();
	if (i == 60) {
		spi_write(0x7C, 0x14);
		spi_write(0x6C, 0x00);
		spi_write(0x7F, 0x00);
	}
	spi_write(0x22, 0x00);
	spi_write(0x55, 0x00);
	spi_write(0x7F, 0x00);
	spi_write(0x40, 0x00);

	// 6.1.7
	(void)spi_read(0x02);
	(void)spi_read(0x03);
	(void)spi_read(0x04);
	(void)spi_read(0x05);
	(void)spi_read(0x06);
	ss_high();
	// 7.3
	paw3399_corded_gaming();

	ss_low();
	spi_write(0x5B, 0x20);
	ss_high();

	paw3399_set_dpi(dpi);
	paw3399_set_as(ang_snap);
	paw3399_set_lod(lod);
}

