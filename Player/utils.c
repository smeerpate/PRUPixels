#include "utils.h"
#include "video.h"

void getPixelRGB(AVFrame *RGBFrame, int x, int y, int maxWidth, int maxHeight, uint32_t *RGB)
{
    if ((x < maxWidth) && (y < maxHeight))
	{
        int offset = y * RGBFrame->linesize[0] + x * 3;
        uint32_t r = (uint32_t)RGBFrame->data[0][offset];
        uint32_t g = (uint32_t)RGBFrame->data[0][offset + 1];
        uint32_t b = (uint32_t)RGBFrame->data[0][offset + 2];
        *RGB = (r << 16) + (g << 24) + (b << 8);
    }
	else
	{
		*RGB = 0;
	}
}