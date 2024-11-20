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

#define LED_PIN 27

void app_main(void) {
    configure_rmt(LED_PIN);

    setRainbow(84, 1);

    //fillAnimation(10, 0x00FF00);

    while (1) {
        vTaskDelay(portMAX_DELAY);  // Keep the main task alive
    }
} 