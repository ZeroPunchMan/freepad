#include "button.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "systime.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

typedef struct
{
    ButtonStatus_t status;
    uint32_t downTime;
    uint32_t upTime;
} ButtonContext_t;

static const struct gpio_dt_spec buttons[ButtonEnum_Max] = {
    [ButtonEnum_Func] = GPIO_DT_SPEC_GET_OR(DT_PATH(buttons, func_button), gpios, {0}),
};

static ButtonContext_t buttonContext[ButtonEnum_Max];

void Button_Init(void)
{
    for (int i = 0; i < ButtonEnum_Max; i++)
    {
        if (!gpio_is_ready_dt(&buttons[i]))
        {
            printk("Error: button device %s is not ready\n",
                   buttons[i].port->name);
            return;
        }

        int ret = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
        if (ret != 0)
        {
            printk("Error %d: failed to configure %s pin %d\n",
                   ret, buttons[i].port->name, buttons[i].pin);
            return;
        }
    }

    memset(buttonContext, 0, sizeof(buttonContext));
}

void Button_Check(uint32_t interval)
{
    for (int i = 0; i < ButtonEnum_Max; i++)
    {
        if (!gpio_pin_get_dt(&buttons[i]))
        {
            buttonContext[i].upTime += interval;
            buttonContext[i].downTime = 0;

            if (buttonContext[i].upTime >= 20)
            {
                // if (buttonContext[i].status == ButtonSta_Press)
                //     printk("button: %d release\r\n", i);

                buttonContext[i].status = ButtonSta_Realse;
            }
            // printk("button: %d high\r\n", i);
        }
        else
        {
            buttonContext[i].upTime = 0;
            buttonContext[i].downTime += interval;

            if (buttonContext[i].downTime >= 20)
            {
                // if (buttonContext[i].status == ButtonSta_Realse)
                //     printk("button: %d press\r\n", i);
                    
                buttonContext[i].status = ButtonSta_Press;
            }
            // printk("button: %d low\r\n", i);
        }
    }
}

ButtonStatus_t Button_GetStatus(ButtonEnum_t btn)
{
    return buttonContext[btn].status;
}
