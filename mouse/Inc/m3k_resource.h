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

#define M3K_USB_MFG "Zaunkoenig"
#define M3K_USB_NAME "M3K"
#define M3K_FW_VERSION " fw v0.0"
#define M3K_USB_PID 0xA462
#define M3K_USB_VID 0x0483

// SPI periph pin clocks
#define SPIx                             SPI3
#define SPIx_CLK_ENABLE()                do {RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;} while(0)
#define SPIx_SCK_GPIO_CLK_ENABLE()       do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;} while(0)
#define SPIx_MISO_GPIO_CLK_ENABLE()      do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;} while(0)
#define SPIx_MOSI_GPIO_CLK_ENABLE()      do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;} while(0)
#define SPIx_SS_GPIO_CLK_ENABLE()	     do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;} while(0)

// SPI
#define SPIx_SCK_GPIO_PORT  GPIOC
#define SPIx_SCK_PIN_Pos    10
#define SPIx_SCK_PIN        (1 << SPIx_SCK_PIN_Pos)
#define SPIx_SCK_AF         6 //GPIO_AF6_SPI3

#define SPIx_MISO_GPIO_PORT GPIOB
#define SPIx_MISO_PIN_Pos   4
#define SPIx_MISO_PIN       (1 << SPIx_MISO_PIN_Pos)
#define SPIx_MISO_AF        6 //GPIO_AF6_SPI3

#define SPIx_MOSI_GPIO_PORT GPIOB
#define SPIx_MOSI_PIN_Pos   2
#define SPIx_MOSI_PIN       (1 << SPIx_MOSI_PIN_Pos)
#define SPIx_MOSI_AF        7 //GPIO_AF6_SPI3

#define SPIx_SS_PORT        GPIOC
#define SPIx_SS_PIN_Pos     9
#define SPIx_SS_PIN         (1 << SPIx_SS_PIN_Pos)

// 3399 NRESET
#define NRESET_GPIO_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;} while(0)
#define NRESET_PORT    GPIOA
#define NRESET_PIN_Pos 8
#define NRESET_PIN     (1 << NRESET_PIN_Pos)

// lmb
#define LMB_NO_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;} while(0)
#define LMB_NO_PORT         GPIOE
#define LMB_NO_PIN_Pos      9
#define LMB_NO_PIN          (1 << LMB_NO_PIN_Pos)
#define LMB_NO_EXTICFG      SYSCFG_EXTICR3_EXTI9_PE

#define LMB_NC_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;} while(0)
#define LMB_NC_PORT         GPIOC
#define LMB_NC_PIN_Pos      2
#define LMB_NC_PIN          (1 << LMB_NC_PIN_Pos)
#define LMB_NC_EXTICFG      SYSCFG_EXTICR1_EXTI2_PC

// rmb
#define RMB_NO_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;} while(0)
#define RMB_NO_PORT         GPIOE
#define RMB_NO_PIN_Pos      3
#define RMB_NO_PIN          (1 << RMB_NO_PIN_Pos)
#define RMB_NO_EXTICFG      SYSCFG_EXTICR1_EXTI3_PE

#define RMB_NC_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;} while(0)
#define RMB_NC_PORT         GPIOE
#define RMB_NC_PIN_Pos      6
#define RMB_NC_PIN          (1 << RMB_NC_PIN_Pos)
#define RMB_NC_EXTICFG      SYSCFG_EXTICR2_EXTI6_PE

// mmb
#define MMB_NO_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;} while(0)
#define MMB_NO_PORT         GPIOA
#define MMB_NO_PIN_Pos      11
#define MMB_NO_PIN          (1 << MMB_NO_PIN_Pos)
#define MMB_NO_EXTICFG      SYSCFG_EXTICR3_EXTI11_PA

#define MMB_NC_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;} while(0)
#define MMB_NC_PORT         GPIOA
#define MMB_NC_PIN_Pos      12
#define MMB_NC_PIN          (1 << MMB_NC_PIN_Pos)
#define MMB_NC_EXTICFG      SYSCFG_EXTICR4_EXTI12_PA

// wheel
#define WHL_P_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;} while(0)
#define WHL_P_PORT         GPIOA
#define WHL_P_PIN_Pos      7
#define WHL_P_PIN          (1 << WHL_P_PIN_Pos)
#define WHL_P_EXTICFG      SYSCFG_EXTICR2_EXTI7_PA

#define WHL_N_CLK_ENABLE() do {RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;} while(0)
#define WHL_N_PORT         GPIOC
#define WHL_N_PIN_Pos      0
#define WHL_N_PIN          (1 << WHL_N_PIN_Pos)
#define WHL_N_EXTICFG      SYSCFG_EXTICR1_EXTI0_PC
