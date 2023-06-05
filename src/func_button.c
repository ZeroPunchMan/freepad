#include "func_button.h"
#include "button.h"
#include "systime.h"
#include <zephyr/sys/reboot.h>
#include "ble_agent.h"

extern void MainSetTask(uint8_t c);

static ButtonStatus_t funcBtnSta = ButtonSta_Realse;
static uint32_t staKeepTime = 0;

void FuncButton_Init(void)
{
}

void FuncButton_Update(uint32_t interval)
{
    ButtonStatus_t curSta = Button_GetStatus(ButtonEnum_Func);
    if (curSta != funcBtnSta)
    {
        if (funcBtnSta == ButtonSta_Realse)
        { // button down
        }
        else if (funcBtnSta == ButtonSta_Press)
        { // button up
            if (staKeepTime < SYSTIME_SECOND(5))
                MainSetTask('u');
        }
        staKeepTime = 0;
        funcBtnSta = curSta;
    }
    else
    {

        if (funcBtnSta == ButtonSta_Press)
        {
            if (staKeepTime < SYSTIME_SECOND(5) && (staKeepTime + interval >= SYSTIME_SECOND(5)))
            {
                MainSetTask('r');
            }
        }
        staKeepTime += interval;
    }
}
