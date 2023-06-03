
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>
#include "systime.h"

volatile static bool initialized = false;

static bool feed = true;
static int wdt_channel_id;
const static struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
void IwdgInit(void)
{
    int err;
    if (!device_is_ready(wdt))
    {
        printk("%s: device not ready.\n", wdt->name);
        return;
    }

    struct wdt_timeout_cfg wdt_config = {
        /* Reset SoC when watchdog timer expires. */
        .flags = WDT_FLAG_RESET_SOC,

        /* Expire watchdog after max window */
        .window.min = 0,
        .window.max = 3500,
        .callback = NULL,
    };

    wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    if (wdt_channel_id < 0)
    {
        printk("Watchdog install error\n");
        return;
    }

    err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (err < 0)
    {
        printk("Watchdog setup error\n");
        return;
    }

    initialized = true;
}

void IwdgProcess(void)
{
    if (!initialized)
        return;

    static uint32_t lastTime = 0;
    if (SysTimeSpan(lastTime) >= 1000)
    {
        lastTime = GetSysTime();
        if (feed)
        {
            // printk("iwdg feed\r\n");
            wdt_feed(wdt, wdt_channel_id);
        }
    }
}

void IwdgDontFeed(void)
{
    feed = false;
}

void IwdgForceFeed(void)
{
    if (!initialized)
        return;

    wdt_feed(wdt, wdt_channel_id);
}
