/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include "systime.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "hog.h"
#include "ble_agent.h"

//*****************button***********************
#define BTN1_NODE DT_PATH(buttons, unpair_button)
#if !DT_NODE_HAS_STATUS(BTN1_NODE, okay)
#error "button not set"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(BTN1_NODE, gpios,
															  {0});
static struct gpio_callback button_cb_data;

static bool buttonPress = false;
void button_pressed(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());

	buttonPress = true;
}

void ButtonInit(void)
{
	if (!gpio_is_ready_dt(&button))
	{
		printk("Error: button device %s is not ready\n",
			   button.port->name);
		return;
	}

	int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0)
	{
		printk("Error %d: failed to configure %s pin %d\n",
			   ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
										  GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0)
	{
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			   ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);
}

//------------------end of button-------------------------

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
			 "Console device is not ACM CDC UART device");

void main(void)
{
	ButtonInit();

	// const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	// uint32_t dtr = 0;

	if (usb_enable(NULL))
	{
		return;
	}

	/* Poll if the DTR flag was set */
	// while (!dtr) {
	// 	uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
	// 	/* Give CPU resources to low priority threads. */
	// 	k_sleep(K_MSEC(100));
	// }

	while (1)
	{
		static uint32_t lastTime = 0;
		if (SysTimeSpan(lastTime) >= 1000)
		{
			lastTime = GetSysTime();
			// printk("Hello World! %u\n", GetSysTime() / 1000);
		}
		static int step = 0;

		if (step == 1)
			BleAgent_Process();

		if (buttonPress)
		{
			buttonPress = false;

			switch (step)
			{
			case 0:
				BleAgent_Init();
				step = 1;
				break;
			case 1:
				BleAgent_Unbond();
				break;
			default:
				break;
			}
		}

		k_sleep(K_MSEC(1));
	}
}
