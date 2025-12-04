#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <libavformat/avformat.h>

void getPixelRGB(AVFrame *RGBFrame, int x, int y, int maxWidth, int maxHeight, uint32_t *RGB);

#endif // UTILS_H
