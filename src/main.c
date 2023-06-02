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
#include <zephyr/sys/reboot.h>
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
#include "iwdg.h"

//*****************button***********************
#define BTN1_NODE DT_PATH(buttons, func_button)
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
const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
volatile static uint8_t cmd = 0;

#define RECEIVE_BUFF_SIZE 10
#define RECEIVE_TIMEOUT 100

static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type)
	{

	case UART_RX_RDY:
		uint8_t c = evt->data.rx.buf[evt->data.rx.offset];
		printk("uart rx rdy: %02x\r\n", c);
		cmd = c;
		// sys_reboot(SYS_REBOOT_WARM);
		break;
	case UART_RX_DISABLED:
		uart_rx_enable(dev, rx_buf, sizeof rx_buf, RECEIVE_TIMEOUT);
		printk("uart rx disable\r\n");
		break;

	default:
		break;
	}
}

void main(void)
{
	if (!device_is_ready(uart))
	{
		printk("UART device not ready\r\n");
		return;
	}

	int ret = uart_callback_set(uart, uart_cb, NULL);
	if (ret)
	{
		return;
	}

	ret = uart_rx_enable(uart, rx_buf, sizeof rx_buf, RECEIVE_TIMEOUT);
	if (ret)
	{
		return;
	}
	
	ButtonInit();
	IwdgInit();
	// if (usb_enable(NULL))
	// {
	// 	return;
	// }

	while (1)
	{
		static uint32_t lastTime = 0;
		if (SysTimeSpan(lastTime) >= 1000)
		{
			lastTime = GetSysTime();
			// printk("Hello World! %u\n", GetSysTime() / 1000);
		}

		BleAgent_Process();
		IwdgProcess();

		if (buttonPress)
		{
			buttonPress = false;
		}

		if (cmd != 0)
		{
			switch (cmd)
			{
			case 'b':
				BleAgent_Init();
				break;
			case 'u':
				BleAgent_Unbond();
				break;
			case 'd': //DFU by WDG reset
				IwdgDontFeed();
				break;
			case 'r':
				sys_reboot(SYS_REBOOT_WARM); // just reset
				break;
			default:
				break;
			}

			cmd = 0;
		}

		k_sleep(K_MSEC(1));
	}
}
