/*


gcc -o player player.c $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil)

sudo ./player

To install this file use this command:
	sudo install -m 755 player /usr/local/sbin/

*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

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
#define OUTWIDTH 150
#define OUTHEIGHT 150


void get_pixel_rgb(AVFrame *rgb_frame, int x, int y, uint32_t *RGB);
bool isPRURunning();


int main() 
{
	void *pruSharedMemPointer, *virt_addr;
	uint32_t pixels[NPIXELS];
	uint32_t RGB;
	double playbackStartTime, frameTimestamp, currentTime; // voor mp4 afspeelsnelheid
	
    // Video openen
	const char *filename = "video.mp4";
	
	while (1) // loop playback
	{
		if (isPRURunning())
		{
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
			int nBufferBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, OUTWIDTH, OUTHEIGHT, 1);
			uint8_t *pixelBuffer = (uint8_t *)av_malloc(nBufferBytes);
			printf("[INFO] %d bytes gealloceerd voor de RGB buffer.\n", nBufferBytes);
			av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, pixelBuffer, AV_PIX_FMT_RGB24, OUTWIDTH, OUTHEIGHT, 1);
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
			pruSharedMemPointer = mmap(NULL, PRU_SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PRU_SHARED_MEM_PHYS & ~MAP_MASK);
			if (pruSharedMemPointer == MAP_FAILED)
			{
				perror("[ERROR] mmap");
				close(mem_fd);
				return 1;
			}
			printf("[INFO] Memory mapped at address %p.\n", pruSharedMemPointer); 
			fflush(stdout);

			playbackStartTime = av_gettime_relative() / 1000000.0; // huidige tijd in seconden (nodig voor synchronisatie
			
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
							
							for(int i=0; i<NPIXELS; i++)
							{
								get_pixel_rgb(rgb_frame, i%OUTWIDTH, 30, &RGB);
								((unsigned long *) pruSharedMemPointer)[i] = RGB;
							}
							
							// wacht tot het tijd is om de volgende frame af te spelen
							frameTimestamp = frame->pts * av_q2d(video_stream->time_base); // frame timestamp in seconden
							currentTime = (av_gettime_relative() / 1000000.0) - playbackStartTime;
							if (frameTimestamp > currentTime)
							{
								double delay = frameTimestamp - currentTime;
								//av_usleep((int64_t)(delay * 1000000));
								av_usleep((int64_t)(delay * 100000));
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
		
			// FFMPEG Cleanup
			av_free(pixelBuffer);
			av_frame_free(&frame);
			av_frame_free(&rgb_frame);
			sws_freeContext(sws_ctx);
			avcodec_free_context(&codec_ctx);
			avformat_close_input(&fmt_ctx);

			
			printf("[INFO] Video geschreven naar PRU shared memory.\n");

			munmap(pruSharedMemPointer, PRU_SHARED_MEM_SIZE);
			close(mem_fd);
		}
		else
		{
			av_usleep(3000000);
		}
	}

    return 0;
}

//////// Functions //////////
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

bool isPRURunning()
{
	char state[64];
	
    FILE *pruStateFile = fopen("/sys/class/remoteproc/remoteproc1/state", "r");
    if (pruStateFile == NULL)
    {
        printf("[ERROR] Can't open PRU state!\n");
        return false;
    }

    if (fgets(state, sizeof(state), pruStateFile) == NULL)
    {
        printf("[ERROR] Can't read PRU state!\n");
        fclose(pruStateFile);
        return false;
    }
    fclose(pruStateFile);

    // Remove newline and carriage return
    state[strcspn(state, "\r\n")] = '\0';

    if (strcmp(state, "running") == 0)
    {
        return true;
    }
    else
    {
        printf("[INFO] PRU state is: %s\n", state);
        return false;
    }
}

