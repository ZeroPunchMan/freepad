#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "usb_agent.h"


#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void dfu(void)
{
	while (true)
	{
		uint8_t buff[64];
		uint32_t recvLen = RecvData(buff, sizeof(buff));
		if (recvLen)
		{
			SendData(buff, recvLen);
		}
	}

}


void blink(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led))
	{
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		return ;
	}

	while (1)
	{
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0)
		{
			return ;
		}

		k_msleep(1000);
	}
	return ;
}


#define STACKSIZE 1024
K_THREAD_DEFINE(blink_id, STACKSIZE, blink, NULL, NULL, NULL,
				1, 0, 0);

K_THREAD_DEFINE(dfu_id, STACKSIZE, dfu, NULL, NULL, NULL,
				2, 0, 0);
