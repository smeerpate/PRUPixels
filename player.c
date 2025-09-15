/*


gcc -o player player.c $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil)

sudo ./player

*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

// FFMPEG libs
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>


#define PRU_SHARED_MEM_PHYS 0x4A310000
#define PRU_SHARED_MEM_SIZE 0x3000  // 12 KB
#define MAP_MASK (PRU_SHARED_MEM_SIZE - 1)

#define NPIXELS 1200
#define OUTWIDTH 100
#define OUTHEIGHT 100


void get_pixel_rgb(AVFrame *rgb_frame, int x, int y, uint32_t *RGB)
{
	if((x < OUTWIDTH) && (y < OUTHEIGHT))
	{
		int offset = y * rgb_frame->linesize[0] + x * 3;
		uint32_t r = (uint32_t)rgb_frame->data[0][offset];
		uint32_t g = (uint32_t)rgb_frame->data[0][offset + 1];
		uint32_t b = (uint32_t)rgb_frame->data[0][offset + 2];
		*RGB = (r << 16) + (g << 24) + (b << 8);
	} 
}


int main() 
{
	void *map_base, *virt_addr;
	uint32_t pixels[NPIXELS];
	uint32_t RGB;
	double start_time, frame_time, now; // voor mp4 afspeelsnelheid
	
    // Video openen
	const char *filename = "video.mp4";

	AVFormatContext *fmt_ctx = NULL;
	if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0)
	{
        printf("[ERROR] kon file %s niet openen\n", filename);
        return -1;
    }
	
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
	{
        printf("[ERROR] Kon streaminfo niet vinden.\n");
        return -1;
    }

	int video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_index < 0)
	{
        printf("[ERROR] Kon geen video stream vinden.\n");
        return -1;
    }
	
	AVStream *video_stream = fmt_ctx->streams[video_stream_index];
	AVCodecParameters *codecpar = video_stream->codecpar;
	//AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
	AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
	AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codec_ctx, codecpar);
	avcodec_open2(codec_ctx, codec, NULL);
	
	
	AVRational framerate = av_guess_frame_rate(fmt_ctx, video_stream, NULL);
    printf("[INFO] File: %s (%dpx x %dpx / %.2ffps)\n", filename, codec_ctx->width, codec_ctx->height, av_q2d(framerate));

	
	// Maak een Software Scaling Context aan
	struct SwsContext *sws_ctx = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        OUTWIDTH, OUTHEIGHT, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL
    );
	
	AVFrame *frame = av_frame_alloc(); // de AVFrame structuren bevatten enkel pointers naar de video en audio data
    AVFrame *rgb_frame = av_frame_alloc();
	
	// maak een RGB buffer aan
	int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, OUTWIDTH, OUTHEIGHT, 1);
	uint8_t *rgb_buffer = (uint8_t *)av_malloc(num_bytes);
	printf("[INFO] %d bytes gealloceerd voor de RGB buffer.\n", num_bytes);
	av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, rgb_buffer, AV_PIX_FMT_RGB24, OUTWIDTH, OUTHEIGHT, 1);
	printf("[INFO] rgb_frame->linesize = %d\n", rgb_frame->linesize[0]);
	
	// Open /dev/mem
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0)
	{
        perror("[ERROR] open");
        return 1;
    }
	printf("[INFO] /dev/mem opened...\n"); 
    fflush(stdout);
	
	
    // Memory map PRU shared memory
    map_base = mmap(NULL, PRU_SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PRU_SHARED_MEM_PHYS & ~MAP_MASK);
    if (map_base == MAP_FAILED)
	{
        perror("[ERROR] mmap");
        close(mem_fd);
        return 1;
    }
	printf("[INFO] Memory mapped at address %p.\n", map_base); 
    fflush(stdout);
	virt_addr = map_base;

	start_time = av_gettime_relative() / 1000000.0; // huidige tijd in seconden (nodig voor synchronisatie
	
	AVPacket packet; // AVPacket bevat: Gecodeerde data en Metadata zoals stream_index, pts, dts, enz.
    while (av_read_frame(fmt_ctx, &packet) >= 0) // lees MP4-bestandpakketten zolang er zijn
	{
		if (packet.stream_index == video_stream_index) // ingelezen pakket (AVPacket) behoort tot de videostream?
		{
			if (avcodec_send_packet(codec_ctx, &packet) == 0) // het versturen van een gecodeerd pakket naar de decoder succesvol is verlopen?
			{
				// Nu kunnen we proberen een frame te ontvangen
				while (avcodec_receive_frame(codec_ctx, frame) == 0)
				{
					sws_scale(sws_ctx, (uint8_t const * const *)frame->data, frame->linesize, 0, codec_ctx->height, rgb_frame->data, rgb_frame->linesize); // scale het frame en sla op in RBG buffer
/* 					get_pixel_rgb(rgb_frame, 50, 30, &RGB);
					((unsigned long *) virt_addr)[0] = RGB;
					get_pixel_rgb(rgb_frame, 51, 30, &RGB);
					((unsigned long *) virt_addr)[1] = RGB;
					get_pixel_rgb(rgb_frame, 52, 30, &RGB);
					((unsigned long *) virt_addr)[2] = RGB;
					get_pixel_rgb(rgb_frame, 53, 30, &RGB);
					((unsigned long *) virt_addr)[3] = RGB; */
					for(int i=0; i<NPIXELS; i++)
					{
						get_pixel_rgb(rgb_frame, i%OUTWIDTH, 30, &RGB);
						((unsigned long *) virt_addr)[i] = RGB;
					}
					
					// wacht tor het tijd is om de volgende frame af te spelen
					frame_time = frame->pts * av_q2d(video_stream->time_base); // frame timestamp in seconden
					now = (av_gettime_relative() / 1000000.0) - start_time;
					if (frame_time > now)
					{
						double delay = frame_time - now;
						av_usleep((int64_t)(delay * 1000000));
					}
					else
					{
						printf("[WARNING] video loopt niet in sync!\n");
						fflush(stdout);
					}
					
					//printf("%d ", frame->linesize[0]);
					//fflush(stdout);
				}
			}
		}
		av_packet_unref(&packet); // AVPacket geheugen vrijmaken, kan >200kB zijn
	}
	
	// Cleanup
    av_free(rgb_buffer);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

	
/*      // Open /dev/mem
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0)
	{
        perror("[ERROR] open");
        return 1;
    }
	printf("[INFO] /dev/mem opened...\n"); 
    fflush(stdout);
	
	
    // Memory map PRU shared memory
    map_base = mmap(NULL, PRU_SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PRU_SHARED_MEM_PHYS & ~MAP_MASK);
    if (map_base == MAP_FAILED)
	{
        perror("[ERROR] mmap");
        close(mem_fd);
        return 1;
    }
	printf("[INFO] Memory mapped at address %p.\n", map_base); 
    fflush(stdout);

	
	// Schrijf pixels naar PRU shared memory
	virt_addr = map_base;
    for (int i = 0; i < NPIXELS; i++)
	{
		((unsigned long *) virt_addr)[i] = pixels[i];  // RGBA pixel
		//printf("0x%X ", pixels[i]);
    } */
	
	
	printf("[INFO] Beeldje geschreven naar PRU shared memory.\n");

    munmap(map_base, PRU_SHARED_MEM_SIZE);
    close(mem_fd);

    return 0;
}

