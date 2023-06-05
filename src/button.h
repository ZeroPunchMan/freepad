#pragma once

#include "stdint.h"
typedef enum
{
    ButtonSta_Realse = 0,
    ButtonSta_Press,
} ButtonStatus_t;

typedef enum
{
    ButtonEnum_Func = 0,
    ButtonEnum_Max,
}  ButtonEnum_t;

void Button_Init(void);
void Button_Check(uint32_t interval);

ButtonStatus_t Button_GetStatus(ButtonEnum_t btn);
