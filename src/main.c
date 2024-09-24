#include <zephyr/kernel.h>

#include "usb_agent.h"

int main(void)
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

	return 0;
}
