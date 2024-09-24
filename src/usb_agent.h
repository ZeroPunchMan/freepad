#pragma once

#include "stdint.h"

int UsbAgent_Init(void);
uint32_t SendData(const uint8_t *data, uint32_t len);
uint32_t RecvData(uint8_t *buffer, uint32_t len);
