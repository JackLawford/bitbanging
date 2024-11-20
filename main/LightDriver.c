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

#define RMT_CHANNEL RMT_CHANNEL_0
#define RMT_CLK_DIV 1        // 12.5 ns ticks (80 MHz clock with clk_div = 1)
#define RESET_PULSE_DURATION 6784  // 50 µs reset pulse for WS2812B in ticks

#define T1H 64               // 800 ns for a logical "1" high
#define T1L 36               // 450 ns for a logical "1" low
#define T0H 32               // 400 ns for a logical "0" high
#define T0L 68               // 850 ns for a logical "0" low

static const char *TAG = "RMT_LED";

// Initialize RMT for sending LED data
void configure_rmt(uint8_t gpio_pin) {
    rmt_config_t rmt_cfg = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_CHANNEL,
        .gpio_num = gpio_pin,
        .clk_div = RMT_CLK_DIV,
        .mem_block_num = 1,
        .tx_config = {
            .loop_en = false,
            .carrier_en = false,
            .idle_output_en = true,
            .idle_level = RMT_IDLE_LEVEL_LOW,
        }
    };
    ESP_ERROR_CHECK(rmt_config(&rmt_cfg));
    ESP_ERROR_CHECK(rmt_driver_install(RMT_CHANNEL, 0, 0));
}

// Convert a color to an RMT signal in GRB format
void set_color(uint32_t color, rmt_item32_t *items) {
    led24 led;
    led.bits.green = (color >> 16) & 0xFF;
    led.bits.red = (color >> 8) & 0xFF;
    led.bits.blue = color & 0xFF;

    for (int i = 0; i < 8; i++) {
        // Green channel
        items[i].level0 = 1;
        items[i].duration0 = (led.bits.green & (1 << (7 - i))) ? T1H : T0H;
        items[i].level1 = 0;
        items[i].duration1 = (led.bits.green & (1 << (7 - i))) ? T1L : T0L;

        // Red channel
        items[8 + i].level0 = 1;
        items[8 + i].duration0 = (led.bits.red & (1 << (7 - i))) ? T1H : T0H;
        items[8 + i].level1 = 0;
        items[8 + i].duration1 = (led.bits.red & (1 << (7 - i))) ? T1L : T0L;

        // Blue channel
        items[16 + i].level0 = 1;
        items[16 + i].duration0 = (led.bits.blue & (1 << (7 - i))) ? T1H : T0H;
        items[16 + i].level1 = 0;
        items[16 + i].duration1 = (led.bits.blue & (1 << (7 - i))) ? T1L : T0L;
    }
}

// Send colors to all LEDs with reset
void setFrame(Frame newFrame) {
    // Allocate enough RMT items for each LED plus one for the reset pSulse
    rmt_item32_t *items = (rmt_item32_t *)malloc((newFrame.num_leds * 24 + 1) * sizeof(rmt_item32_t));
    if (items == NULL) {
        return; // Not enough memory to send the frame
    }

    // Set the color for each LED
    for (int i = 0; i < newFrame.num_leds; i++) {
        set_color(newFrame.leds[i].val, &items[i * 24]);
    }

    // Add a reset pulse at the end of the sequence
    items[newFrame.num_leds * 24].level0 = 0;
    items[newFrame.num_leds * 24].duration0 = RESET_PULSE_DURATION;
    items[newFrame.num_leds * 24].level1 = 0;
    items[newFrame.num_leds * 24].duration1 = 0;

    // Send the color data to the RMT channel
    ESP_ERROR_CHECK(rmt_write_items(RMT_CHANNEL, items, newFrame.num_leds * 24 + 1, false));
    ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_CHANNEL, portMAX_DELAY));  // Wait for the transmission to complete

    // Clean up
    free(items);
}

void setSolid(uint32_t color, uint32_t num_leds) {
    Frame currFrame = {
        .num_leds = num_leds,
        .leds = (led24 *)malloc(num_leds * sizeof(led24))
    };

    for (int i = 0; i < num_leds; i++) {
        currFrame.leds[i].val = color;
    }

    setFrame(currFrame);

    // Clean up
    free(currFrame.leds);
}

void setSweeping(uint32_t color, uint32_t num_leds, uint32_t delay) {
    Frame currFrame = {
        .num_leds = num_leds,
        .leds = (led24 *)malloc(num_leds * sizeof(led24))
    };

    int direction = 1;
    int position = 0;

    // struct timeval tv_now;
    // gettimeofday(&tv_now, NULL);
    // int64_t startTime = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    
    while (1) {
        // Turn off all LEDs
        memset(currFrame.leds, 0, num_leds * sizeof(led24));

        // Set the current position to the input color
        currFrame.leds[position].val = color;

        // Load the array to the strip
        setFrame(currFrame);

        // Move it
        position += direction;

        // Change direction at the ends
        if (position == num_leds - 1 || position == 0) {
            direction = -direction;  // Change direction
        }

        // Delay to control the speed of the sweep
        vTaskDelay(pdMS_TO_TICKS(delay));
    }

    // gettimeofday(&tv_now, NULL);
    // int64_t endTime = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    // ESP_LOGI(TAG, "Time taken: %lld", endTime - startTime);

    // Clean up
    free(currFrame.leds);
}

void setRainbow(uint32_t num_leds, uint32_t delay) {
    Frame currFrame = {
        .num_leds = num_leds,
        .leds = (led24 *)malloc(num_leds * sizeof(led24))
    };

    int position = 0;

    // Set the LEDs to black
    memset(currFrame.leds, 0, num_leds * sizeof(led24));

    for (int i = 0; i < num_leds; i++) {
        currFrame.leds[i].val = (uint32_t)HSVtoRGB((float)i / num_leds, 1.0, 1.0);

        setFrame(currFrame);
    }

    while (1) {

        currFrame.leds[position].val = currFrame.leds[(position + 1) % num_leds].val;

        // Set the current position to the rainbow color
        setFrame(currFrame);

        // Move it
        position = (position + 1) % num_leds;

        // Delay to control the speed of the sweep
        vTaskDelay(pdMS_TO_TICKS(delay));
    }

    // Clean up
    free(currFrame.leds);
}

void fillAnimation(uint32_t delay, uint32_t color) {
    Frame currFrame = {
        .num_leds = 420,
        .leds = (led24 *)malloc(420 * sizeof(led24))
    };

    int i = 0;
    int j = 0;
    int layerZeroEdges[] = {1, 12, 13, 24, 25};
    int layerOneEdges[] = {2, 11, 14, 23, 26};
    int layerTwoEdges[] = {3, 4, 9, 10, 15, 16, 21, 22, 27, 28};
    int layerThreeEdges[] = {5, 8, 17, 20, 29};
    int layerFourEdges[] = {6, 7, 18, 19, 30};

    while (1) {
        switch (i) {
            case 0:
                for (j = 0; j < 14; j++) {
                    currFrame.leds[((layerZeroEdges[0] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerZeroEdges[1] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerZeroEdges[2] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerZeroEdges[3] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerZeroEdges[4] * 14) - 14) + j].val = color;
                }
                setFrame(currFrame);
                vTaskDelay(pdMS_TO_TICKS(delay));
                i++;
                break;
            case 1:
                for (j = 0; j < 12; j++) {
                    currFrame.leds[((layerOneEdges[0] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerOneEdges[1] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerOneEdges[2] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerOneEdges[3] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerOneEdges[4] * 14) - 14) + j].val = color;
                }
                setFrame(currFrame);
                vTaskDelay(pdMS_TO_TICKS(delay));
                i++;
                break;
            case 2:
                for (j = 0; j < 18; j++) {
                    currFrame.leds[((layerTwoEdges[0] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[1] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[2] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[3] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[4] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[5] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[6] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[7] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[8] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerTwoEdges[9] * 14) - 14) + j].val = color;
                }
                setFrame(currFrame);
                vTaskDelay(pdMS_TO_TICKS(delay));
                i++;
                break;
            case 3:
                for (j = 0; j < 10; j++) {
                    currFrame.leds[((layerThreeEdges[0] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerThreeEdges[1] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerThreeEdges[2] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerThreeEdges[3] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerThreeEdges[4] * 14) - 14) + j].val = color;
                }
                setFrame(currFrame);
                vTaskDelay(pdMS_TO_TICKS(delay));
                i++;
                break;
            case 4:
                for (j = 0; j < 10; j++) {
                    currFrame.leds[((layerFourEdges[0] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerFourEdges[1] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerFourEdges[2] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerFourEdges[3] * 14) - 14) + j].val = color;
                    currFrame.leds[((layerFourEdges[4] * 14) - 14) + j].val = color;
                }
                setFrame(currFrame);
                vTaskDelay(pdMS_TO_TICKS(delay));
                i = 0;
                break;
            default:
                break;
        }
    }
}
