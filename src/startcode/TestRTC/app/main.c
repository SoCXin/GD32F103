#include "app.h"
#include "app_rtc.h"

int main()
{
    (VOID)DRV_SYSTICK_Init();
    APP_DEBUG_Init();
    
    APP_RTC_Test();
}
