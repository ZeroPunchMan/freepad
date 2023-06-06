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
#include <zephyr/sys/reboot.h>
#include "button.h"
#include "func_button.h"
#include <zephyr/drivers/pwm.h>

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

void MainSetTask(uint8_t c)
{
	cmd = c;
}

void ButtonTimerHandler(struct k_timer *dummy)
{
	ARG_UNUSED(dummy);
	Button_Check(10);
	FuncButton_Update(10);
}

K_TIMER_DEFINE(buttonTimer, ButtonTimerHandler, NULL);

//**************pwm****************
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

void PwmInit(void)
{
	if (!device_is_ready(pwm_led0.dev))
	{
		printk("Error: PWM device %s is not ready\n",
			   pwm_led0.dev->name);
		return;
	}

	uint32_t max_period = PWM_SEC(1U) / 128U;
	pwm_set_dt(&pwm_led0, max_period, max_period / 2U);
}

//-------------pwm------------------

void main(void)
{
	PwmInit();

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

	IwdgInit();
	Button_Init();
	// FuncButton_Init();

	k_timer_start(&buttonTimer, K_MSEC(10), K_MSEC(10));
	// if (usb_enable(NULL))
	// {
	// 	return;
	// }

	while (1)
	{
		static uint32_t lastTime = 0;
		if (SysTimeSpan(lastTime) >= 10)
		{
			lastTime = GetSysTime();

			static uint32_t duty = 0;
			static bool inc = true;
			uint32_t max_period = PWM_SEC(1U) / 128U;
			uint32_t period = max_period / 200 * duty;
			pwm_set_dt(&pwm_led0, max_period, period);

			inc ? duty++ : duty--;
			if (duty >= 200)
				inc = false;
			if (duty == 0)
				inc = true;
			// uint32_t resetReason = *((volatile uint32_t*)(0x40000000 +  0x400));
			// printk("Hello World! %u\n", resetReason);
		}

		BleAgent_Process();
		IwdgProcess();

		if (cmd != 0)
		{
			switch (cmd)
			{
			case 'b':
				BleAgent_Init();
				break;
			case 'u':
				// BleAgent_Unbond();
				printk("unbond\r\n");
				break;
			case 'd': // DFU by WDG reset
				IwdgDontFeed();
				break;
			case 'r':
				printk("reset\r\n");
				// IwdgForceFeed();
				// sys_reboot(SYS_REBOOT_WARM); // just reset
				break;
			default:
				break;
			}

			cmd = 0;
		}

		k_sleep(K_MSEC(1));
	}
}
