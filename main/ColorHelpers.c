#include "ColorHelpers.h"
#include <math.h>

// Convert HSV to RGB
uint32_t HSVtoRGB(float h, float s, float v) {
    float r, g, b;
    int i;
    float f, p, q, t;

    if (s == 0) {
        r = g = b = v;
    } else {
        h *= 6;
        i = (int)h;
        f = h - i;
        p = v * (1 - s);
        q = v * (1 - s * f);
        t = v * (1 - s * (1 - f));

        switch (i) {
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = t;
                g = p;
                b = v;
                break;
            default:
                r = v;
                g = p;
                b = q;
                break;
        }
    }

    return ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | (int)(b * 255);
}