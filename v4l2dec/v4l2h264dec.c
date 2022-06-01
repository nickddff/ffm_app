/*
 * Copyright (C)
 *
 * 	qipengzhen <aric.pzqi@ingenic.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/media.h>
#include <linux/videodev2.h>

#include "v4l2h264dec.h"

/****************config****************/
#define VIDEO_PATH "/dev/video2"
#define STREAM_FILE "tt.h264"
#define FORMAT_SELECT "nv12"
#define SAVE_DECODED_FRAME 0
#define PREVIEW_DECODED_FRAME 1
#define SHOW_RATE 1
#define OUTPUT_FORMAT_DEBUG 0 

//#define PIC_WIDTH	1280           //in stream pix
//#define PIC_HEIGHT	720
//#define FB_WIDTH	480
//#define FB_HEIGHT	800

//#define FB_WIDTH	480
//#define FB_HEIGHT	800
//#define DISPLAY_WIDTH	240
//#define DISPLAY_HEIGHT	400

//#define DISPLAY_WIDTH	240
//#define DISPLAY_HEIGHT	400

//#define PACKET_LEN	10*1024*1024
//#define PACKET_LEN	512 * 1024
//char packet[PACKET_LEN];


int fb_fd;
char *fb_mapaddr;
int fb_w;
int fb_h;
int display_w;
int display_h;
int pic_w;
int pic_h;
int save_decoded_frame = 0;
int preview_decoded_frame = 1;
int show_frame_rate = 1;



struct format_description formats[] = {
	{
		.description		= "nv12",
		.v4l2_format		= V4L2_PIX_FMT_NV12,
		.v4l2_buffers_count	= 2,
		.v4l2_mplane		= true,
		.planes_count		= 2,
	},
	{
		.description		= "nv21",
		.v4l2_format		= V4L2_PIX_FMT_NV21,
		.v4l2_buffers_count	= 2,
		.v4l2_mplane		= true,
		.planes_count		= 2,
	},
	{
		.description		= "tile420",
		.v4l2_format		= V4L2_PIX_FMT_JZ420B,
		.v4l2_buffers_count	= 2,
		.v4l2_mplane		= true,
		.planes_count		= 2,
	},
	{
		.description		= "rgb888",
		.v4l2_format		= V4L2_PIX_FMT_RGB24,
		.v4l2_buffers_count	= 2,
		.v4l2_mplane		= true,
		.planes_count		= 1,
	},
};

static void print_help(void)
{
	printf("Usage: v4l2-request-test [OPTIONS] [SLICES PATH]\n\n"
	       "Options:\n"
	       " -v [video path]                path for the video node\n"
	       " -f                             input h264 stream filename\n"
	       " -q                             enable quiet mode\n"
	       " -w                             pic width\n"
	       " -h                             pic height\n"
	       " -t                             formats: nv12, nv21, tile420\n"
	       " -n				number of buffers to use\n"
	       " -i				interactive mode, paused after decode one frame until getchar\n"
	       " -s				save decode frame to file.\n"
	       " -r				show decode frame rate\n"
	       " -H                             help\n\n"
	       "Video presets:\n");

}

static void print_summary(struct config *config, struct preset *preset)
{
	printf("Config:\n");
	printf(" Video path: %s\n", config->video_path);
	printf(" stream_path: %s\n", config->stream_path);
	printf(" Buffers count: %d\n", config->buffers_count);
	printf(" interactive_mode: %d\n", config->interactive_mode);
	printf("\n\n");
}

typedef struct {
	unsigned char *stream;
	unsigned char type;
	unsigned int size;
} nal_unit_t;
#define MAX_NALS_PER_TIME       900	//25fps@60min
typedef struct {
        nal_unit_t nal[MAX_NALS_PER_TIME];
        int nal_count;
} nal_units_t;

nal_units_t nal_units;



static int m_find_start_code(unsigned char *buf, unsigned int len, unsigned int *prefix_bytes)
{
        unsigned char *start = buf;
        unsigned char *end = buf + len;
        int index = 0;


        if(start >= end)
                return -1;

        /* TODO: check the condition of end. */
        /* evaluate the whole walk through time, optimise. */


        //printf("============start_code: start: %x, end: %x\n", start, end);
        //      start += 3;
        while(start < end) {
#if 1
                /* next24bits */
                index = start - buf;

                if (start[0] == 0 && start[1] == 0 && start[2] == 1) {
                        *prefix_bytes = 3;
                        return index + 3;
                }
#if 0
                /* next32bits */
                if (start[0] == 0 && start[1] == 0 && start[2] == 0 &&
                                start[3] == 1) {
                        *prefix_bytes = 4;
                        return index + 4;
                }
#endif

                start++;


                /* end of data buffer, no need to find. */
		if((start + 3) == end) {
                        return -1;
                }
#endif
        }

        return -1;
        //      return start - data;
}

static unsigned int get_nal_size(char *buf, unsigned int len)
{
        char *start = buf;
        char *end = buf + len;
	unsigned int size = 0;

        if(start >= end)
                return -1;

        while(start < end) {

                if (start[0] == 0 && start[1] == 0 && start[2] == 1) {
			break;
                }

                start++;
        }

	size = (unsigned int) (start - buf);

        return size;
}


void hexdump(unsigned char *buf, int len)
{
        int i;

        for (i = 0; i < len; i++) {
                if ((i % 16) == 0)
                        printf("%s%08x: ", i ? "\n" : "",
                                        (unsigned int)&buf[i]);
                printf("%02x ", buf[i]);
        }
        printf("\n");
}


static void dump_nal_units(nal_units_t *nals)
{
	int i;
	nal_unit_t *nal;

	for(i = 0; i < nals->nal_count; i++) {
		printf("nal: %d\n", i);
		nal = &nals->nal[i];
		hexdump(nal->stream, nal->size < 32 ? nal->size : 32);
	}

}

int extract_nal_units(nal_units_t *nals, unsigned char *buf, unsigned int len)
{
	int index = 0;
	int next_nal = 0;
	unsigned char *start = buf;
	unsigned char *end = buf + len;
	int size = 0;
	unsigned int prefix_bytes; /*000001/00000001*/
	nal_unit_t *nal;

	unsigned int left = len;
	int i = 0;

	int eos = 0;
	nals->nal_count = 0;

    for(i = 0; i < MAX_NALS_PER_TIME; i++) {

        nal = &nals->nal[i];
		index = m_find_start_code(start, left, &prefix_bytes);
		if(index < 0 ) {
			printf("no start code found!\n");
			return -EINVAL;
		}

		nal->stream = start;
		nal->type = nal->stream[index];

//		start += index;
//		left -= index;
		nal->size = get_nal_size(start + index, left - index) + index;
		nals->nal_count++;

		start += nal->size;
		left -= nal->size;

        if(left <= 0) {
            /*EOS*/
            break;
        }


#if 0
		next_nal = m_find_start_code(start, left, &prefix_bytes);
		printf("----2 find start code @%d, prefix_bytes: %d\n", next_nal, prefix_bytes);
		if(next_nal < 0) {
			printf("End of stream %d\n", next_nal);
			/* no next start code found. means end of nal.*/
			size = left;
			next_nal = 0;
			prefix_bytes = 0;
			eos = 1;
		} else {
			size = next_nal - prefix_bytes; /*有效的rbsp大小.*/
		}

		//nal->stream = start - prefix_bytes;
		nal->type = nal->stream[prefix_bytes];
		//printf("------nal->stream[%d]: %x\n", prefix_bytes, nal->stream[prefix_bytes]);
		nal->size = size + prefix_bytes;

		start += size;
		left -= size;

		if(eos)
			break;
#endif

    }

	if(left) {
		printf("too many nals extraced!, %d bytes left\n", left);
	}

    //dump_nal_units(nals);

	return len - left;
}

struct timeval get_timeval_diff(struct timeval tv_0, struct timeval tv_1){
	struct timeval delt_tv;
	if(tv_1.tv_usec >= tv_0.tv_usec){
		delt_tv.tv_usec = tv_1.tv_usec - tv_0.tv_usec;
		delt_tv.tv_sec = tv_1.tv_sec - tv_0.tv_sec;
	} else {
		delt_tv.tv_usec = tv_1.tv_usec + 1000000 - tv_0.tv_usec;
		delt_tv.tv_sec = tv_1.tv_sec - 1 - tv_0.tv_sec;
	}
	return delt_tv;
}


struct timeval accumulate_timeval_diff(struct timeval tv_sum, struct timeval tv){
	tv_sum.tv_usec += tv.tv_usec;
	tv_sum.tv_sec += tv.tv_sec;
	if(tv_sum.tv_usec >= 1000000){
		tv_sum.tv_usec -= 1000000;
		tv_sum.tv_sec += 1;
	}
	return tv_sum;
}

float get_frame_rate(struct timeval tv_sum, int count){
	float time_sum = tv_sum.tv_sec * 1000000 + tv_sum.tv_usec;
	float fps = count*1000000 / time_sum;
	return fps;
}

static long time_diff(struct timespec *before, struct timespec *after)
{
	long before_time = before->tv_sec * 1000000 + before->tv_nsec / 1000;
	long after_time = after->tv_sec * 1000000 + after->tv_nsec / 1000;

	return (after_time - before_time);
}

static void print_time_diff(struct timespec *before, struct timespec *after,
			    const char *prefix)
{
	long diff = time_diff(before, after);
	printf("%s time: %ld us\n", prefix, diff);
}

static int load_data(const char *path, void **data, unsigned int *size)
{
	void *buffer = NULL;
	unsigned int length;
	struct stat st;
	int fd;
	int rc;

	rc = stat(path, &st);
	if (rc < 0) {
		fprintf(stderr, "Stating file failed\n");
		goto error;
	}

	length = st.st_size;

	buffer = malloc(length);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Unable to open file path: %s\n",
			strerror(errno));
		goto error;
	}

	rc = read(fd, buffer, length);
	if (rc < 0) {
		fprintf(stderr, "Unable to read file data: %s\n",
			strerror(errno));
		goto error;
	}

	close(fd);

	*data = buffer;
	*size = length;

	rc = 0;
	goto complete;

error:
	if (buffer != NULL)
		free(buffer);

	rc = -1;

complete:
	return rc;
}

static void setup_config(struct config *config)
{
	memset(config, 0, sizeof(*config));
    config->video_path = strdup(VIDEO_PATH);   //  "/dev/video2"
    config->stream_path = strdup(STREAM_FILE);   //  "tt.h264"
    strcpy(config->format,FORMAT_SELECT);        //
    config->buffers_count = 1;
	config->interactive_mode = 0;
    save_decoded_frame = SAVE_DECODED_FRAME;  //default 0
    preview_decoded_frame = PREVIEW_DECODED_FRAME; //default 1
    show_frame_rate = SHOW_RATE;  //default 0
}


static void cleanup_config(struct config *config)
{
	free(config->video_path);
	free(config->stream_path);
}


struct preset *preset;
struct config config;
struct video_buffer *video_buffers;
struct video_setup video_setup;
struct timespec before, after;
struct format_description *selected_format = NULL;
void *slice_data = NULL;
unsigned int slice_size;
unsigned int width;
unsigned int height;
unsigned int v4l2_index = 0;

unsigned int i;
int video_fd = -1;
uint64_t ts;
bool test;

struct fb_var_screeninfo varinfo;
struct timeval tv1, tv2, delt_tv,tv_start,tv_end,tsv0,tsv1;
struct timeval sum_tv = {0};
int timeval_count = 0;
float fps = 0;

int V4l2dec_videoplay_init(int width_t,int height_t)
{

    int rc;
	setup_config(&config);

//	width = PIC_WIDTH;
//	height = PIC_HEIGHT;
    width = width_t;
    height = height_t;
    /**********/

    pic_w = width;
    pic_h = height;
	print_summary(&config, preset);

    video_fd = open(config.video_path, O_RDWR, 0);
	if (video_fd < 0) {
		fprintf(stderr, "Unable to open video node: %s\n",
			strerror(errno));
		goto error;
	}
	//printf("debug address2 \r\n");
	if (preview_decoded_frame == 1) {
        fb_fd = open("/dev/fb0", O_RDWR);
		if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &varinfo) == -1) {
			perror("open fb error\n");
			return -EINVAL;
		}
		//printf("debug address3 \r\n");
        fb_w = varinfo.xres;
        fb_h = varinfo.yres;
//                fb_w = 1280;
//                fb_h = 720;

		display_w = fb_w;
		display_h = fb_h;
		printf("in---> pic_w: %d, pic_h: %d | out---> display_w: %d, display_h: %d\n", pic_w, pic_h, display_w, display_h);
	}


	/*默认map 一个帧buffer, 按照每个像素4byte去算.*/
	fb_mapaddr=(char*)mmap(0, fb_w * fb_h *4, PROT_WRITE | PROT_READ, MAP_SHARED, fb_fd, 0);  //建立fdfd的map shared

    if(config.format != "") {
        for(i = 0; i < ARRAY_SIZE(formats); i++) {
            if(!strcmp(config.format, formats[i].description)) {
                selected_format = &formats[i];
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < ARRAY_SIZE(formats); i++) {
            test = video_engine_format_test(video_fd,
                    formats[i].v4l2_mplane, width,
                    height, formats[i].v4l2_format);
            if (test) {
                selected_format = &formats[i];
                break;
            }
        }
    }

	if (selected_format == NULL) {
		fprintf(stderr,
			"Unable to find any supported destination format\n");
		goto error;
	}

	printf("Destination format: %s\n", selected_format->description);

	test = video_engine_capabilities_test(video_fd, V4L2_CAP_STREAMING);
	if (!test) {
		fprintf(stderr, "Missing required driver streaming capability\n");
		goto error;
	}


	/*初始化v4l2相关配置.*/
	rc = video_engine_start(video_fd, width, height,
				selected_format, CODEC_TYPE_H264, &video_buffers,
				&config.buffers_count, &video_setup);
	if (rc < 0) {
		printf("debug address7 \r\n");
		fprintf(stderr, "Unable to start video engine\n");
		goto error;
	}
	printf("debug address8 \r\n");
	rc = 0;
	goto complete;

error:
    rc = 1;
	printf("v4l2 init into err\r\n");
	exit(EXIT_FAILURE);
    return rc;

complete:
	printf("v4l2 init success\n");
	gettimeofday(&tv_start,NULL);
	gettimeofday(&tsv0,NULL);
	return 0;
}


int v4l2dec_handling_perfame(unsigned char *packet,int packet_len,uint64_t timer)
{

        int ret = 0,rc = 0;
        nal_unit_t *nal = NULL;
        slice_size = 0;
        int nal_count = 0;
        int need_merge = 0;
        nal_units.nal_count = 0;

        ret = extract_nal_units(&nal_units, packet, packet_len);
        //ret = extract_nal_units(&nal_units, packet, packet_len);
//        if(ret < packet_len) {
//            printf("packet not extracted completely!\n");
//        }
        nal_count = nal_units.nal_count;

            nal = &nal_units.nal[0];

            for(i = 0; i < nal_count; i++) {
                nal = &nal_units.nal[i];
                /*Merge SPS/PPS/Filler/IDR. */

                if((nal->type & 0x1f) == 0x7) { //SPS/IDR. {
                    need_merge = 1;
                    slice_data = nal->stream;
                    slice_size = nal->size;
                    continue;
                }
                if((nal->type & 0x1f) == 0x8) {   //pps
                    slice_size += nal->size;
                    continue;
                }
                if((nal->type & 0x1f) == 0xc) {	//filler na, 0xff
                    slice_size += nal->size;
                    continue;
                }
                if((nal->type & 0x1f) == 0x06) { //vui
                    slice_size += nal->size;
                    continue;
                }

                if((nal->type & 0x1f) == 0x5 && need_merge) {
                    need_merge = 0;
                    slice_size += nal->size;
                }
                else {
                    slice_data = nal->stream;
                    slice_size = nal->size;
                }

                /*decode nal units */
                if(show_frame_rate){
                    gettimeofday(&tv1, NULL);
					//printf("fd is %d\n",video_fd);
                    rc = video_engine_decode(video_fd, v4l2_index,
                            CODEC_TYPE_H264, ts, slice_data,
                            slice_size, video_buffers,
                            &video_setup,timer);
                    gettimeofday(&tv2, NULL);
                    delt_tv = get_timeval_diff(tv1, tv2);
                    sum_tv = accumulate_timeval_diff(sum_tv, delt_tv);
                    timeval_count ++;
                } else {
                    rc = video_engine_decode(video_fd, v4l2_index,
                            CODEC_TYPE_H264, ts, slice_data,
                            slice_size, video_buffers,
                            &video_setup,timer);
                }
                if (rc < 0) {
                    fprintf(stderr, "Unable to decode video frame\n");
                    return rc;
                }

                v4l2_index ++;
                v4l2_index %= config.buffers_count;
                /*display frame*/
            }
    return rc;
}


// int v4l2dec_handling(unsigned char *packet,int packet_len)
// {

//     int ret = 0,rc = 0;
//     /*
//     int fd;
//     fd = open(config.stream_path, O_RDONLY);
//     if(fd < 0) {
//         perror("failed to open stream!");
//         return -1;
//     }
//     unsigned long total_size = lseek(fd, 0, SEEK_END);
//     lseek(fd, 0, SEEK_SET); */

//     //int packet_len = 0;
//     nal_unit_t *nal = NULL;
//     nal_unit_t *last_nal = NULL;
//     unsigned long current_offset = 0;

//     //while(1) {
//         slice_size = 0;
//         int need_merge = 0;
//         int nal_count = 0;

// //		packet_len = read(fd, packet, PACKET_LEN);
// //		if(packet_len <= 0) {
// //			break;
// //		}

// 		nal_units.nal_count = 0;
//         ret = extract_nal_units(&nal_units, packet, packet_len);
//         //ret = extract_nal_units(&nal_units, packet, packet_len);
// 		if(ret < packet_len) {
// 			printf("packet not extracted completely!\n");
// 		}
//         nal_count = nal_units.nal_count;



//         /*
//          *

//         last_nal = &nal_units.nal[nal_units.nal_count - 1];
//         current_offset = lseek(fd, 0, SEEK_CUR);
// 		if(current_offset < total_size) {
// 			nal_count = nal_units.nal_count - 1;
// 			lseek(fd, -last_nal->size, SEEK_CUR);
//             //printf("move:%d curr:%ld totle:%ld nalcount:%d\r\n",last_nal->size,current_offset,total_size,nal_count);
// 		} else {

// 			nal_count = nal_units.nal_count;
//         } */			/*EOF.*/

//         printf("nalcount:%d...\r\n",nal_count);
// 		for(i = 0; i < nal_count; i++) {
// 			nal = &nal_units.nal[i];
// 			/*Merge SPS/PPS/Filler/IDR. */

//             if((nal->type & 0x1f) == 0x7) { //SPS/
// 				need_merge = 1;
// 				slice_data = nal->stream;
// 				slice_size = nal->size;
// 				continue;
// 			}
//             if((nal->type & 0x1f) == 0x8) {  //PS/
// 				slice_size += nal->size;
// 				continue;
// 			}
// 			if((nal->type & 0x1f) == 0xc) {	//filler na, 0xff
// 				slice_size += nal->size;
// 				continue;
// 			}
// 			if((nal->type & 0x1f) == 0x06) { //vui
// 				slice_size += nal->size;
// 				continue;
// 			}

//             if((nal->type & 0x1f) == 0x5 && need_merge) {
// 				need_merge = 0;
// 				slice_size += nal->size;
// 			} else {
// 				slice_data = nal->stream;
// 				slice_size = nal->size;
// 			}

// 			/*decode nal units */
// 			if(show_frame_rate){
// 				gettimeofday(&tv1, NULL);
// 				rc = video_engine_decode(video_fd, v4l2_index,
// 						CODEC_TYPE_H264, ts, slice_data,
// 						slice_size, video_buffers,
// 						&video_setup);
// 				gettimeofday(&tv2, NULL);
// 				delt_tv = get_timeval_diff(tv1, tv2);
// 				sum_tv = accumulate_timeval_diff(sum_tv, delt_tv);
// 				timeval_count ++;
// 			 } else {
// 			 	rc = video_engine_decode(video_fd, v4l2_index,
// 						CODEC_TYPE_H264, ts, slice_data,
// 						slice_size, video_buffers,
// 						&video_setup);
// 			 }

// 			if (rc < 0) {
// 				fprintf(stderr, "Unable to decode video frame\n");
//                 return rc;
// 			}

// 			if(config.interactive_mode == 1) {
// 				while(1) {
// 					printf("input 'c' to continue:\n");
// 					char c = getchar();
// 					if(c == 'c') {
// 						break;
// 					}
// 				}
// 			}

// 			v4l2_index ++;
// 			v4l2_index %= config.buffers_count;
// 			/*display frame*/
// 			}
//     return rc;
// }

int v4l2dec_stop(void)
{
    int rc = 0;
	struct timeval del;
    rc = video_engine_stop(video_fd, video_buffers, config.buffers_count,&video_setup);
	gettimeofday(&tv_end,NULL);
	del = get_timeval_diff(tv_start,tv_end);
	printf("total_tv : %d sec, %d usec\n", del.tv_sec, del.tv_usec);
	if(show_frame_rate){
		fps = get_frame_rate(sum_tv, timeval_count);
		printf("sum_tv : %d sec, %d usec\n", sum_tv.tv_sec, sum_tv.tv_usec);
		printf("count = %d\n", timeval_count);
		printf("frame rate : %.02f fps\n", fps);
	}
	if (rc < 0) {
		fprintf(stderr, "Unable to stop video engine\n");
	}
	if (video_fd >= 0)close(video_fd);
	munmap(fb_mapaddr, fb_w*fb_h*4);
	if(fb_fd > 0)close(fb_fd);
	cleanup_config(&config);
	exit(EXIT_FAILURE);
	return rc;

}
