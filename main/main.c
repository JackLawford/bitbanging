#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "esp_log.h"
#include <sys/time.h>
#include "sdkconfig.h"
#include <string.h>
#include "LightDriver.h"
#include "ColorHelpers.h"

#define LED_PIN 27

void app_main(void) {
    configure_rmt(LED_PIN);

    //setRainbow(420, 1);

    fillAnimation(50, 0x0000FF);

    while (1) {
        vTaskDelay(portMAX_DELAY);  // Keep the main task alive
    }
}      