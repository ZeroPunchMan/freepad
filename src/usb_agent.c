#include "usb_agent.h"

#include <sample_usbd.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/ring_buffer.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>

typedef struct
{
    bool init;
} UsbAgentContext_t;

volatile static UsbAgentContext_t context = {
    .init = false,
};

LOG_MODULE_REGISTER(cdc_acm_echo, LOG_LEVEL_INF);

const struct device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

static uint8_t recvBuffer[1024];
static uint8_t sendBuffer[1024];

static struct ring_buf recvRingBuffer;
static struct ring_buf sendRingBuffer;

static void interrupt_handler(const struct device *dev, void *user_data)
{ // thread usbworkq; k_is_in_isr()->false  线程优先级-1,协作式,近似ISR,
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_is_pending(dev))
    {
        if (uart_irq_rx_ready(dev))
        { // 接受中断
            int recv_len;
            uint8_t buffer[64];
            size_t len = MIN(ring_buf_space_get(&recvRingBuffer),
                             sizeof(buffer));

            if (len == 0)
            {
                /* Throttle because ring buffer is full */
                uart_irq_rx_disable(dev);
                break;
            }

            recv_len = uart_fifo_read(dev, buffer, len);
            if (recv_len > 0)
            {
                ring_buf_put(&recvRingBuffer, buffer, recv_len);
            };
        }

        if (uart_irq_tx_ready(dev))
        { // 发送中断
            static uint8_t sb[64];
            static int rb_len = 0, send_len = 0;

            // rb_len = ring_buf_get(&sendRingBuffer, sb, sizeof(sb));
            // if (rb_len)
            // {
            //     send_len = uart_fifo_fill(dev, sb, rb_len);
            // }
            // else
            // {
            //     uart_irq_tx_disable(dev);
            //     break;
            // }

            if (send_len >= rb_len)
            { // 之前数据已经发完,拉新的数据
                rb_len = ring_buf_get(&sendRingBuffer, sb, sizeof(sb));
                if (rb_len)
                { // 拉取到数据,清零已发字节数
                    send_len = 0;
                }
                else
                { // 没拉取到数据,关闭发送中断
                    uart_irq_tx_disable(dev);
                    continue;
                }
            }
            else
            { // send_len < rb_len,还未发完
                send_len += uart_fifo_fill(dev, sb + send_len, rb_len - send_len);
            }
        }
    }
}

uint32_t SendData(const uint8_t *data, uint32_t len)
{
    if (!context.init)
        return 0;
    uint32_t sendLen = ring_buf_put(&sendRingBuffer, data, len);

    uart_irq_tx_enable(uart_dev);
    return sendLen;
}

uint32_t RecvData(uint8_t *buffer, uint32_t len)
{
    if (!context.init)
        return 0;

    uint32_t readLen;

    readLen = ring_buf_get(&recvRingBuffer, buffer, len);

    if (readLen)
    {
        uart_irq_rx_enable(uart_dev);
    }
    return readLen;
}

int UsbAgent_Init(void)
{
    int ret;
    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("CDC ACM device not ready");
        return 0;
    }

    ret = usb_enable(NULL);

    if (ret != 0)
    {
        LOG_ERR("Failed to enable USB");
        return 0;
    }

    ring_buf_init(&recvRingBuffer, sizeof(recvBuffer), recvBuffer);
    ring_buf_init(&sendRingBuffer, sizeof(sendBuffer), sendBuffer);

    LOG_INF("Wait for DTR");

    while (true)
    {
        uint32_t dtr = 0U;

        uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
        if (dtr)
        {
            break;
        }
        else
        {
            /* Give CPU resources to low priority threads. */
            k_sleep(K_MSEC(100));
        }
    }

    LOG_INF("DTR set");

    /* They are optional, we use them to test the interrupt endpoint */
    ret = uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DCD, 1);
    if (ret)
    {
        LOG_WRN("Failed to set DCD, ret code %d", ret);
    }

    ret = uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DSR, 1);
    if (ret)
    {
        LOG_WRN("Failed to set DSR, ret code %d", ret);
    }

    /* Wait 100ms for the host to do all settings */
    k_msleep(100);

    uart_irq_callback_set(uart_dev, interrupt_handler);

    /* Enable rx interrupts */
    uart_irq_rx_enable(uart_dev);

    context.init = true;
    return 0;
}

SYS_INIT(UsbAgent_Init, APPLICATION, 10);
