/* MIT License
 *
 * Copyright (c) 2022 Zaunkoenig GmbH
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

#include <stdint.h>
#include "anim.h"

#define BUF_SIZE 128
static struct Anim buf[BUF_SIZE] = {0}; // circular fifo buffer
static int anim_buf_head = 0, anim_buf_tail = 0; // tail is one past end index
static int anim_time_scale = 1; // slow down animation by this factor

void anim_set_scale(int scale)
{
	anim_time_scale = scale;
}

void anim_add(int reps, int len_seq, const struct Anim seq[])
{
	for (int i = 0; i < reps; i++) {
		for (int j = 0; j < len_seq; j++) {
			buf[anim_buf_tail] = seq[j];
			anim_buf_tail = (anim_buf_tail + 1) % BUF_SIZE;
		}
	}
}


#define HSPACE 180
#define LEN_SEG 150
#define U_SEG ANIM(LEN_SEG / SPEED_SLOW,  0, -SPEED_SLOW)
#define D_SEG ANIM(LEN_SEG / SPEED_SLOW,  0,  SPEED_SLOW)
#define L_SEG ANIM(LEN_SEG / SPEED_SLOW, -SPEED_SLOW,  0)
#define R_SEG ANIM(LEN_SEG / SPEED_SLOW,  SPEED_SLOW,  0)

void anim_num(const uint16_t x)
{
	const struct Anim *symbols[] = {
		(struct Anim[]){D_SEG, D_SEG, R_SEG, U_SEG, U_SEG, L_SEG, ANIM(1, HSPACE, 0)}, // 0
		(struct Anim[]){D_SEG, D_SEG, ANIM(1, HSPACE - LEN_SEG, -LEN_SEG*2)}, // 1
		(struct Anim[]){R_SEG, D_SEG, L_SEG, D_SEG, R_SEG, ANIM(1, HSPACE - LEN_SEG, -LEN_SEG*2)}, // 2
		(struct Anim[]){R_SEG, D_SEG, L_SEG, R_SEG, D_SEG, L_SEG, ANIM(1, HSPACE, -LEN_SEG*2)}, // 3
		(struct Anim[]){D_SEG, R_SEG, U_SEG, D_SEG, D_SEG, ANIM(1, HSPACE - LEN_SEG, -LEN_SEG*2)}, // 4
		(struct Anim[]){L_SEG, D_SEG, R_SEG, D_SEG, L_SEG, ANIM(1, HSPACE, -LEN_SEG*2)}, //5
		(struct Anim[]){L_SEG, D_SEG, D_SEG, R_SEG, U_SEG, L_SEG, ANIM(1, HSPACE, -LEN_SEG)}, // 6
		(struct Anim[]){R_SEG, D_SEG, D_SEG, ANIM(1, HSPACE - LEN_SEG, -LEN_SEG*2)}, // 7
		(struct Anim[]){D_SEG, R_SEG, D_SEG, L_SEG, U_SEG, R_SEG, U_SEG, L_SEG, ANIM(1, HSPACE, 0)}, // 8
		(struct Anim[]){L_SEG, D_SEG, R_SEG, U_SEG, D_SEG, D_SEG, ANIM(1, HSPACE - LEN_SEG, -LEN_SEG*2)} // 9
	};
	const int lengths[] = {7, 3, 6, 7, 6, 6, 7, 4, 9, 7};
	const int right[] = {0, 1, 0, 0, 0, 1, 1, 0, 0, 1}; // 1, 5, 6, 9 start from top right.

	const int powers[] = {10000, 1000, 100, 10, 1};
	const int len_powers = 5;
	int n = 0; // which number we're on, from left to right
	for (int i = 0; i < len_powers; i++) {
		int d = (x / powers[i]) % 10;
		if (n == 0 && d > 0) n = 1;
		if (n > 0 || i == len_powers - 1) {
			if (right[d] && n > 1) // start on right side for 1, 5, 6, 9
				anim_add(1, 1, (struct Anim[]){ANIM(1, LEN_SEG, 0)});
			anim_add(1, lengths[d] - 1, symbols[d]); // symbol apart from shift at the end
			if (i < len_powers - 1) { // except for last digit, pause and then shift
				anim_pause(300);
				anim_add(1, 1, symbols[d] + lengths[d] - 1); // including the shift after a pause
			}
			n++;
		}
	}
}

struct Xy anim_read(void)
{
	if (anim_buf_head == anim_buf_tail)
		return (struct Xy){0};

	static int count = 0;
	count = (count + 1) % anim_time_scale;
	if (count > 0)
		return (struct Xy){0};

	// step through animation only after anim_time_scale calls to anim_read()
	struct Xy ret = buf[anim_buf_head].xy;
	buf[anim_buf_head].len--;
	if (buf[anim_buf_head].len == 0)
		anim_buf_head = (anim_buf_head + 1) % BUF_SIZE;
	return ret;
}
