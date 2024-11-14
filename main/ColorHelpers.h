#include <stdint.h>

/**
 * @brief Convert an HSV color to an RGB color
 * 
 * @param h The hue of the color
 * @param s The saturation of the color
 * @param v The value of the color
 */
uint32_t HSVtoRGB(float h, float s, float v);