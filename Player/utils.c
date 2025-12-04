#include "utils.h"
#include "video.h"

void getPixelRGB(AVFrame *RGBFrame, int x, int y, int maxWidth, int maxHeight, uint32_t *RGB, int driveWhite)
{
    if ((x < maxWidth) && (y < maxHeight))
	{
        int offset = y * RGBFrame->linesize[0] + x * 3;
        uint32_t r = (uint32_t)RGBFrame->data[0][offset];
        uint32_t g = (uint32_t)RGBFrame->data[0][offset + 1];
        uint32_t b = (uint32_t)RGBFrame->data[0][offset + 2];
		uint32_t w = 0;
		if (driveWhite > 0)
		{
			w = smallestOfThree(r, g, b);
		}
        *RGB = (r << 16) + (g << 24) + (b << 8) + w;
    }
	else
	{
		*RGB = 0;
	}
}

uint32_t smallestOfThree(uint32_t a, uint32_t b, uint32_t c)
{
    int min = a;
	
    if (b < min) min = b;
    if (c < min) min = c;
	
    return min;
}