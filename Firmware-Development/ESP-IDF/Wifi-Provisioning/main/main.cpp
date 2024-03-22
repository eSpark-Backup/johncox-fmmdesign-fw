#include <stdio.h>
#include "WifiSoftApp.hpp"

WifiProvisioning wifi;

extern "C" void app_main(void)
{
    wifi.softap();
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    printf("Wifi Connected Successfully\n");
}
