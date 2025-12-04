#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <libavformat/avformat.h>

void getPixelRGB(AVFrame *RGBFrame, int x, int y, int maxWidth, int maxHeight, uint32_t *RGB, int driveWhite);
uint32_t smallestOfThree(uint32_t a, uint32_t b, uint32_t c);

#endif // UTILS_H
