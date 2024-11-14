#include <stdint.h>

/**
 * @brief Definition of LED item
 */
typedef union {
    struct {
        uint32_t green : 8;
        uint32_t red : 8;
        uint32_t blue : 8;
        uint32_t reserved : 8;
    } bits;
    uint32_t val; /*!< Equivalent unsigned value for the led24 item */
} led24;

typedef struct {
    uint32_t num_leds;
    led24* leds;
} Frame;

/**
 * @brief Configure the RMT peripheral to drive the LEDs
 * 
 * @param led_pin The GPIO pin to which the LEDs are connected
 */
void configure_rmt(uint8_t gpio_pin);

/**
 * @brief Set the color of all LEDs to a solid color
 * 
 * @param color The color to set the LEDs to
 * @param num_leds The number of LEDs to set
 */
void setSolid(uint32_t color, uint32_t num_leds);

/**
 * @brief Set the color of all LEDs to a sweeping color with a delay
 * 
 * @param color The color to sweep through
 * @param num_leds The number of LEDs to set
 * @param delay The delay between each sweep
 */
void setSweeping(uint32_t color, uint32_t num_leds, uint32_t delay);

/**
 * @brief Set the color of all LEDs to a rainbow pattern
 * 
 * @param num_leds The number of LEDs to set
 * @param delay The delay between each sweep
 */
void setRainbow(uint32_t num_leds, uint32_t delay);
