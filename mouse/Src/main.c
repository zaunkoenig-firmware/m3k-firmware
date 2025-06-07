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

#include <assert.h>
#include <m3k_resource.h>
#include <paw3399.h>
#include <stdint.h>
#include "stm32f7xx.h"
#include "usbd_hid.h"
#include "usb.h"
#include "anim.h"
#include "btn_whl.h"
#include "clock.h"
#include "config.h"
#include "delay.h"

#define TIMEOUT_SECS 5 // seconds of holding buttons for programming mode

int8_t encoder_LUT[4][4][4] = {0}; // LUT for encoder values

typedef union {
	struct __PACKED { // use the order in the report descriptor
		uint8_t btn;
		int8_t whl;
		int16_t x, y;
		uint16_t _pad; // zero pad to 8 bytes total
	};
	uint8_t u8[8]; // btn, wheel, xlo, xhi, ylo, yhi, 0, 0
	uint32_t u32[2];
} Usb_packet;
static_assert(sizeof(Usb_packet) == 2*sizeof(uint32_t), "Usb_packet wrong size");

static Config config_boot(void) {
	// read button state on boot
	uint8_t btn_boot = 0;
	btn_boot |= (!(LMB_NO_PORT->IDR & LMB_NO_PIN)) << 0;
	btn_boot |= (!(RMB_NO_PORT->IDR & RMB_NO_PIN)) << 1;

	// update config depending on initial buttons
	delay_ms(25); // delay in case power bounces on boot
	Config cfg = config_read();
	switch (btn_boot) {
	case 0b01: // LMB pressed
		cfg ^= CONFIG_ANGLE_SNAP_ON;
		config_write(cfg);
		if (cfg & CONFIG_ANGLE_SNAP_ON)
			anim_cw(1);
		else
			anim_ccw(1);
		break;
	case 0b11: // LMB and RMB pressed
		cfg ^= CONFIG_HS_USB;
		config_write(cfg);
		if (cfg & CONFIG_HS_USB)
			anim_eight(1);
		else
			anim_one(1);
		break;
	}

	// systick is enabled only if RMB was held initially (see bootloader)
	if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) {
		cfg = config_default;
		config_write(cfg);
		anim_diag(1);
	}
	return cfg;
}

static inline uint32_t mode_process(Config *cfg, int *skip,
		const uint8_t btn, const uint8_t btn_prev, const uint8_t squal) {
	// mode 0: normal
	// mode 1: cpi programming
	// mode 2: LOD/Hz programming
	static uint32_t mode = 0;
	static uint32_t ticks = 0; // counter for programming mode timeout
	static uint32_t large_step = 0; // ignore releases of the other button for large dpi steps.

	const int hs = ((*cfg & CONFIG_HS_USB) != 0);
	const int timeout_ticks = TIMEOUT_SECS * (hs ? 8000 : 1000);

	// typically squal in 60s for lifted 3399.
	const int SQUAL_THRESH = 75;
	const int lifted = (squal < SQUAL_THRESH);

	const uint16_t dpi_min = 0x0000; // = 0   = 50dpi
	const uint16_t dpi_max = 0x018F; // = 399 = 20000dpi
	const uint16_t DPI_LARGE_JUMP = 10; // 10 * 50dpi = 500.
	uint16_t dpi = _FLD2VAL(CONFIG_DPI, *cfg);

	if (mode == 1) { // handle cpi mode
		const uint8_t released = (~btn) & btn_prev;
		if ((released & 0b01) != 0 && !lifted) { // LMB released
			if (btn & 0b10) { // if RMB is held
				if (dpi != dpi_min) {
					dpi = MAX(dpi - DPI_LARGE_JUMP, dpi_min);
					anim_lg_downup(1);
				}
				large_step = 1;
			} else if (!large_step) {
				if (dpi != dpi_min) {
					dpi--;
					anim_downup(1);
				}
			} else {
				large_step = 0;
			}
			*cfg = (*cfg & (~CONFIG_DPI_Msk)) | dpi;
			paw3399_set_dpi(dpi);
		}
		if ((released & 0b10) != 0 && !lifted) { // RMB released
			if (btn & 0b01) { // if LMB is held
				if (dpi != dpi_max) {
					dpi = MIN(dpi + DPI_LARGE_JUMP, dpi_max);
					anim_lg_updown(1);
				}
				large_step = 1;
			} else if (!large_step) {
				if (dpi != dpi_max) {
					dpi++;
					anim_updown(1);
				}
			} else {
				large_step = 0;
			}
			*cfg = (*cfg & (~CONFIG_DPI_Msk)) | dpi;
			paw3399_set_dpi(dpi);
		}
	} else if (mode == 2) { // handle LOD/Hz mode
		const uint8_t released = (~btn) & btn_prev;
		if ((released & 0b01) != 0 && !lifted) { // LMB released
			const int new_lod = (_FLD2VAL(CONFIG_LOD, *cfg) + 1) % 3;
			*cfg = (*cfg & (~CONFIG_LOD_Msk)) | (new_lod << CONFIG_LOD_Pos);
			anim_cw(1 + new_lod);
			paw3399_set_lod(new_lod);
		}
		if ((released & 0b10) != 0 && !lifted && hs) { // RMB released in HS mode
			// loops 8k (0b00) -> 1k (0b11) -> 2k (0b10) -> 4k (0b01) -> 8k
			const int new_itv = (_FLD2VAL(CONFIG_INTERVAL, *cfg) - 1) % 4;
			*cfg = (*cfg & (~CONFIG_INTERVAL_Msk)) | (new_itv << CONFIG_INTERVAL_Pos);
			*skip = (1 << new_itv) - 1;
			anim_num(1 << (3 - new_itv));
		}
	}

	if (mode == 0) {
		if (lifted && btn == 0b011) {
			ticks++;
			if (ticks == timeout_ticks) {
				// show DPI
				// 10k steps (50 * 200)
                anim_updown_pause((dpi + 1) / 200);
                // 1k steps (50 * 20)
                anim_rightleft_pause(((dpi + 1) % 200) / 20);
                // 100 steps (50 * 2)
                anim_downup_pause((((dpi + 1) % 200) % 20) / 2);
                // 50 steps (50 * 1)
                anim_leftright_pause((((dpi + 1) % 200) % 20) % 2);
			}
			if (ticks == 2*timeout_ticks) {
				anim_cw(1 + _FLD2VAL(CONFIG_LOD, *cfg));
				if (hs) {
					anim_pause(500);
					anim_num(1 << (3 - _FLD2VAL(CONFIG_INTERVAL, *cfg)));
				}
			}
		} else if (ticks < timeout_ticks) {
			ticks = 0;
		} // else, ticks >= timeout_ticks, save the value and wait

		if (btn == 0 && ticks >= timeout_ticks) {
			if (ticks >= 2*timeout_ticks) {
				mode = 2;
			} else { // timeout_ticks <= times <= 2*timeout_ticks
				mode = 1;
			}
		}
	} else { // mode == 1 || mode == 2
		if (lifted && btn == 0b011) {
			ticks++;
			if (ticks == timeout_ticks) {
				if (mode == 1) {
					// show DPI
					// 10k steps (50 * 200)
                    anim_updown_pause((dpi + 1) / 200);
                    // 1k steps (50 * 20)
                    anim_rightleft_pause(((dpi + 1) % 200) / 20);
                    // 100 steps (50 * 2)
                    anim_downup_pause((((dpi + 1) % 200) % 20) / 2);
                    // 50 steps (50 * 1)
                    anim_leftright_pause((((dpi + 1) % 200) % 20) % 2);
				} else if (mode == 2) {
					anim_cw(1 + _FLD2VAL(CONFIG_LOD, *cfg));
					if (hs) {
						anim_pause(500);
						anim_num(1 << (3 - _FLD2VAL(CONFIG_INTERVAL, *cfg)));
					}
				}
				mode = 0;
				config_write(*cfg);
				ticks = 0;
			}
		} else {
			ticks = 0;
		}
	}

	// input mask
	return (mode == 0) ? 0xFFFFFFFF : 0xFFFFFFFC;
}

int main(void) {
//	extern uint32_t _sitcm;
//	SCB->VTOR = (uint32_t)(&_sitcm);
	extern uint32_t _sflash;
	SCB->VTOR = (uint32_t) (&_sflash);
	SCB_EnableICache();
	SCB_EnableDCache();

	clk_init();
	delay_init();
	btn_whl_init();
	uint8_t btn_prev = 0;
	int whl_lastlast = whl_read();
	int whl_last = whl_lastlast;
	int whl_count = 0; // microframe counter for limiting wheel code rate
	Config cfg = config_boot();

	// init encoder LUT
	// initialised to all 0s
	encoder_LUT[0][3][1] = 1;
	encoder_LUT[0][3][2] = -1;
	encoder_LUT[3][0][1] = -1;
	encoder_LUT[3][0][2] = 1;

	const int hs_usb = ((cfg & CONFIG_HS_USB) != 0);
	anim_set_scale(hs_usb ? 8 : 1);
	usb_init(hs_usb);
	usb_wait_configured();

	spi_init();
	paw3399_init(cfg);

	const uint32_t USBx_BASE = (uint32_t) USB_OTG_HS; // used in macros USBx_*
	// fifo space when empty, should equal 0x174, from init_usb
	const uint32_t fifo_space = (USBx_INEP(1)->DTXFSTS
			& USB_OTG_DTXFSTS_INEPTFSAV);

	Usb_packet new = { 0 }; // what's new this loop
	Usb_packet send = { 0 }; // what's transmitted

	int skip = hs_usb ? (1 << _FLD2VAL(CONFIG_INTERVAL, cfg)) - 1 : 0;
	int count = 0; // counter to skip reports

	USB_OTG_HS->GINTMSK |= USB_OTG_GINTMSK_SOFM; // enable SOF interrupt
	while (1) {
		// always check that usb is configured
		usb_wait_configured();

		// wait for SOF to sync to usb frames
		USB_OTG_HS->GINTSTS |= USB_OTG_GINTSTS_SOF;
		__WFI();

		// if full speed usb, delay here to minimize input lag
		if (!hs_usb)
			delay_us(873);

		delay_us(88);

		// read sensor, buttons
		ss_low();
		spi_send(0x16);
		delay_us(2);
		(void) spi_recv(); // motion, not used
		(void) spi_recv(); // observation, not used
		new.u8[2] = spi_recv(); // x lower 8 bits
		new.u8[3] = spi_recv(); // x upper 8 bits
		new.u8[4] = spi_recv(); // y lower 8 bits
		new.u8[5] = spi_recv(); // y upper 8 bits
		const uint8_t squal = spi_recv(); // SQUAL
		ss_high();

		new.whl = 0;

		if (whl_count == 0) {
			const int whl_now = whl_read();
			if (whl_now != whl_last) {
				new.whl = encoder_LUT[whl_lastlast][whl_now][whl_last];
				if (new.whl != 0) {
					whl_lastlast = whl_last;
					whl_last = whl_now;
				}
			}
		}
		if (hs_usb) // only run wheel code every 4 microframes
			whl_count = (whl_count + 1) % 4;

		const uint16_t btn_raw = btn_read();
		const uint8_t btn_NO = (btn_raw & 0xFF);
		const uint8_t btn_NC = (btn_raw >> 8);
		btn_prev = new.btn;
		new.btn = (~btn_NO & 0b111) | (btn_NC & btn_prev);

		// mode processing
		const uint32_t mask = mode_process(&cfg, &skip, new.btn, btn_prev, squal);

		// animation stuff
		const struct Xy a = anim_read(); // returns 0 if no animation left
		new.x += a.x;
		new.y += a.y;

		// if last packet still sitting in fifo
		if ((USBx_INEP(1)->DTXFSTS & USB_OTG_DTXFSTS_INEPTFSAV) < fifo_space) {
			// flush fifo
			USB_OTG_HS->GRSTCTL = _VAL2FLD(USB_OTG_GRSTCTL_TXFNUM,
					1) | USB_OTG_GRSTCTL_TXFFLSH;
			while ((USB_OTG_HS->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH) != 0)
				;
			count = 0; // reset counter, try to transmit again
		} else if (count == skip) { // last loop transmitted successfully
			send.whl = 0;
			send.x = 0;
			send.y = 0;
		}
		send.whl += new.whl;
		send.x += new.x;
		send.y += new.y;

		// skip transmission for "skip" loops after a successful transmission
		if (count > 0) {
			count--;
			continue;
		}

		// if there is data to transmitted
		if (new.btn != send.btn || send.whl || send.x || send.y) {
			send.btn = new.btn;
			// disable transfer complete interrupts, in case enabled by OTG_HS_IRQHandler
			USBx_DEVICE->DIEPMSK &= ~USB_OTG_DIEPMSK_XFRCM;
			// set up transfer size
			MODIFY_REG(USBx_INEP(1)->DIEPTSIZ,
					USB_OTG_DIEPTSIZ_PKTCNT | USB_OTG_DIEPTSIZ_XFRSIZ,
					_VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) | _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, HID_EPIN_SIZE));
			// enable endpoint
			USBx_INEP(1)->DIEPCTL |= USB_OTG_DIEPCTL_CNAK
					| USB_OTG_DIEPCTL_EPENA;
			// write to fifo
			USBx_DFIFO(1) = send.u32[0] & mask;
			USBx_DFIFO(1) = send.u32[1];
			count = skip;
		}
	}
	return 0;
}
