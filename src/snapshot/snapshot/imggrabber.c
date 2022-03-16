/*
 * Copyright (c) 2022 roleo.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Read the h264 i-frame from a file and convert it using libavcodec
 * and libjpeg.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include "libavcodec/avcodec.h"

#include "convert2jpg.h"
#include "add_water.h"

#define FF_INPUT_BUFFER_PADDING_SIZE 32

#define RESOLUTION_LOW  360
#define RESOLUTION_HIGH 1080

#define RESOLUTION_FHD  1080

#define PATH_RES_LOW  "/mnt/mmc/sonoff-hack/etc/wm_res/low/wm_540p_"
#define PATH_RES_HIGH "/mnt/mmc/sonoff-hack/etc/wm_res/high/wm_540p_"

#define W_LOW 640
#define H_LOW 360
#define W_FHD 1920
#define H_FHD 1080

typedef struct {
    int sps_addr;
    int sps_len;
    int pps_addr;
    int pps_len;
    int vps_addr;
    int vps_len;
    int idr_addr;
    int idr_len;
} frame;

struct __attribute__((__packed__)) frame_header {
    uint32_t len;
    uint32_t counter;
    uint32_t time;
    uint16_t type;
    uint16_t stream_counter;
};

int res;
int debug;

int frame_decode(unsigned char *outbuffer, unsigned char *p, int length, int h26x)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    AVFrame *picture;
    int got_picture, len;
    FILE *fOut;
    uint8_t *inbuf;
    AVPacket avpkt;
    int i, j, size;

//////////////////////////////////////////////////////////
//                    Reading H264                      //
//////////////////////////////////////////////////////////

    if (debug) fprintf(stderr, "Starting decode\n");

    av_init_packet(&avpkt);

    if (h26x == 4) {
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            if (debug) fprintf(stderr, "Codec h264 not found\n");
            return -2;
        }
    } else {
        codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
        if (!codec) {
            if (debug) fprintf(stderr, "Codec hevc not found\n");
            return -2;
        }
    }

    c = avcodec_alloc_context3(codec);
    picture = av_frame_alloc();

    if((codec->capabilities) & AV_CODEC_CAP_TRUNCATED)
        (c->flags) |= AV_CODEC_FLAG_TRUNCATED;

    if (avcodec_open2(c, codec, NULL) < 0) {
        if (debug) fprintf(stderr, "Could not open codec h264\n");
        av_free(c);
        return -2;
    }

    // inbuf is already allocated in the main function
    inbuf = p;
    memset(inbuf + length, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    // Get only 1 frame
    memcpy(inbuf, p, length);
    avpkt.size = length;
    avpkt.data = inbuf;

    // Decode frame
    if (debug) fprintf(stderr, "Decode frame\n");
    if (c->codec_type == AVMEDIA_TYPE_VIDEO ||
         c->codec_type == AVMEDIA_TYPE_AUDIO) {

        len = avcodec_send_packet(c, &avpkt);
        if (len < 0 && len != AVERROR(EAGAIN) && len != AVERROR_EOF) {
            if (debug) fprintf(stderr, "Error decoding frame\n");
            return -2;
        } else {
            if (len >= 0)
                avpkt.size = 0;
            len = avcodec_receive_frame(c, picture);
            if (len >= 0)
                got_picture = 1;
        }
    }
    if(!got_picture) {
        if (debug) fprintf(stderr, "No input frame\n");
        av_frame_free(&picture);
        avcodec_close(c);
        av_free(c);
        return -2;
    }

    if (debug) fprintf(stderr, "Writing yuv buffer\n");
    memset(outbuffer, 0x80, c->width * c->height * 3 / 2);
    memcpy(outbuffer, picture->data[0], c->width * c->height);
    for(i=0; i<c->height/2; i++) {
        for(j=0; j<c->width/2; j++) {
            outbuffer[c->width * c->height + c->width * i +  2 * j] = *(picture->data[1] + i * picture->linesize[1] + j);
            outbuffer[c->width * c->height + c->width * i +  2 * j + 1] = *(picture->data[2] + i * picture->linesize[2] + j);
        }
    }

    // Clean memory
    if (debug) fprintf(stderr, "Cleaning ffmpeg memory\n");
    av_frame_free(&picture);
    avcodec_close(c);
    av_free(c);

    return 0;
}

int add_watermark(unsigned char *buffer, int w_res, int h_res)
{
    char path_res[1024];
    FILE *fBuf;
    WaterMarkInfo WM_info;

    if (w_res != W_LOW) {
        strcpy(path_res, PATH_RES_HIGH);
    } else {
        strcpy(path_res, PATH_RES_LOW);
    }

    if (WMInit(&WM_info, path_res) < 0) {
        fprintf(stderr, "water mark init error\n");
        return -1;
    } else {
        if (w_res != W_LOW) {
            AddWM(&WM_info, w_res, h_res, buffer,
                buffer + w_res*h_res, w_res-460, h_res-40, NULL);
        } else {
            AddWM(&WM_info, w_res, h_res, buffer,
                buffer + w_res*h_res, w_res-230, h_res-20, NULL);
        }
        WMRelease(&WM_info);
    }

    return 0;
}

void usage(char *prog_name)
{
    fprintf(stderr, "Usage: %s [options]\n", prog_name);
    fprintf(stderr, "\t-f, --file FILE         Read frame from file FILE\n");
    fprintf(stderr, "\t-r, --res RES           Set resolution: \"low\" or \"high\" (default \"high\")\n");
    fprintf(stderr, "\t-q, --quality Q         Set jpeg quality: 0 - 100 (default 90)\n");
    fprintf(stderr, "\t-w, --watermark         Add watermark to image\n");
    fprintf(stderr, "\t-d, --debug             Enable debug\n");
    fprintf(stderr, "\t-h, --help              Show this help\n");
}

int main(int argc, char **argv)
{
    int errno;
    char *endptr;

    FILE *fHF;

    unsigned char *bufferh26x, *bufferyuv;
    char file[256];
    int quality = -1;
    int watermark = 0;
    int width, height;

    int c;

    struct frame_header fh, fhs, fhp, fhv, fhi;
    unsigned char *fhs_addr, *fhp_addr, *fhv_addr, *fhi_addr;

    int sps_start_found = -1, sps_end_found = -1;
    int pps_start_found = -1, pps_end_found = -1;
    int vps_start_found = -1, vps_end_found = -1;
    int idr_start_found = -1;
    int i, j, f, start_code, is_hevc = 0;
    unsigned char *h26x_file_buffer;
    long h26x_file_size;
    size_t nread;

    memset(file, '\0', sizeof(file));
    res = RESOLUTION_HIGH;
    quality = -1;
    width = W_FHD;
    height = H_FHD;
    debug = 0;

    while (1) {
        static struct option long_options[] = {
            {"file",      required_argument, 0, 'f'},
            {"res",       required_argument, 0, 'r'},
            {"quality",   required_argument, 0, 'q'},
            {"watermark", no_argument,       0, 'w'},
            {"debug",     no_argument,       0, 'd'},
            {"help",      no_argument,       0, 'h'},
            {0,           0,                 0,  0 }
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "f:r:q:wdh",
            long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'f':
                if (strlen(optarg) < sizeof(file)) {
                    strcpy(file, optarg);
                }

            case 'r':
                if (strcasecmp("low", optarg) == 0)
                    res = RESOLUTION_LOW;
                else
                    res = RESOLUTION_HIGH;
                break;

            case 'q':
                errno = 0;    /* To distinguish success/failure after call */
                quality = strtol(optarg, &endptr, 10);

                /* Check for various possible errors */
                if ((errno == ERANGE) && (quality == LONG_MAX || quality == LONG_MIN)) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                if (endptr == optarg) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                if ((quality < 0) || (quality > 100)) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            case 'w':
                watermark = 1;
                break;

            case 'd':
                debug = 1;
                break;

            case 'h':
            default:
                usage(argv[0]);
                exit(-1);
                break;
        }
    }

    if (debug) fprintf(stderr, "Starting program\n");

    if (res == RESOLUTION_LOW) {
        width = W_LOW;
        height = H_LOW;
    } else {
        width = W_FHD;
        height = H_FHD;
    }

    // Read frames from h26x file
    fhs.len = 0;
    fhp.len = 0;
    fhv.len = 0;
    fhi.len = 0;
    fhs_addr = NULL;
    fhp_addr = NULL;
    fhv_addr = NULL;
    fhi_addr = NULL;

    fHF = fopen(file, "r");
    if ( fHF == NULL ) {
        fprintf(stderr, "Could not get size of %s\n", file);
        exit(-6);
    }
    fseek(fHF, 0, SEEK_END);
    h26x_file_size = ftell(fHF);
    fseek(fHF, 0, SEEK_SET);
    h26x_file_buffer = (unsigned char *) malloc(h26x_file_size);
    nread = fread(h26x_file_buffer, 1, h26x_file_size, fHF);
    fclose(fHF);
    if (debug) fprintf(stderr, "The size of the file is %d\n", h26x_file_size);

    if (nread != h26x_file_size) {
        fprintf(stderr, "Read error %s\n", file);
        exit(-7);
    }

    for (f=0; f<h26x_file_size; f++) {
        for (i=f; i<h26x_file_size; i++) {
            if(h26x_file_buffer[i] == 0 && h26x_file_buffer[i+1] == 0 && h26x_file_buffer[i+2] == 0 && h26x_file_buffer[i+3] == 1) {
                start_code = 4;
            } else {
                continue;
            }

            if ((h26x_file_buffer[i+start_code]&0x7E) == 0x40) {
                is_hevc = 1;
                vps_start_found = i;
                break;
            } else if ((h26x_file_buffer[i+start_code]&0x1F) == 0x7) {
                is_hevc = 0;
                sps_start_found = i;
                break;
            } else if ((h26x_file_buffer[i+start_code]&0x7E) == 0x42) {
                is_hevc = 1;
                sps_start_found = i;
                break;
            } else if ((is_hevc == 0) && ((h26x_file_buffer[i+start_code]&0x1F) == 0x8)) {
                pps_start_found = i;
                break;
            } else if ((is_hevc == 1) && ((h26x_file_buffer[i+start_code]&0x7E) == 0x44)) {
                pps_start_found = i;
                break;
            } else if (((h26x_file_buffer[i+start_code]&0x1F) == 0x5) || ((h26x_file_buffer[i+start_code]&0x7E) == 0x26)) {
                idr_start_found = i;
                break;
            }
        }
        for (j = i + 4; j<h26x_file_size; j++) {
            if (h26x_file_buffer[j] == 0 && h26x_file_buffer[j+1] == 0 && h26x_file_buffer[j+2] == 0 && h26x_file_buffer[j+3] == 1) {
                start_code = 4;
            } else {
                continue;
            }

            if ((h26x_file_buffer[j+start_code]&0x7E) == 0x42) {
                vps_end_found = j;
                break;
            } else if ((is_hevc == 0) && ((h26x_file_buffer[j+start_code]&0x1F) == 0x8)) {
                sps_end_found = j;
                break;
            } else if ((is_hevc == 1) && ((h26x_file_buffer[j+start_code]&0x7E) == 0x44)) {
                sps_end_found = j;
                break;
            } else if (((h26x_file_buffer[j+start_code]&0x1F) == 0x5) || ((h26x_file_buffer[j+start_code]&0x7E) == 0x26)) {
                pps_end_found = j;
                break;
            }
        }
        f = j - 1;
    }

    if ((sps_start_found >= 0) && (pps_start_found >= 0) && (idr_start_found >= 0) &&
            (sps_end_found >= 0) && (pps_end_found >= 0)) {

        if ((vps_start_found >= 0) && (vps_end_found >= 0)) {
            fhv.len = vps_end_found - vps_start_found;
            fhv_addr = &h26x_file_buffer[vps_start_found];
        }
        fhs.len = sps_end_found - sps_start_found;
        fhp.len = pps_end_found - pps_start_found;
        fhi.len = h26x_file_size - idr_start_found;
        fhs_addr = &h26x_file_buffer[sps_start_found];
        fhp_addr = &h26x_file_buffer[pps_start_found];
        fhi_addr = &h26x_file_buffer[idr_start_found];

        if (debug) {
            fprintf(stderr, "Found SPS at %d, len %d\n", sps_start_found, fhs.len);
            fprintf(stderr, "Found PPS at %d, len %d\n", pps_start_found, fhp.len);
            if (fhv_addr != NULL) {
                fprintf(stderr, "Found VPS at %d, len %d\n", vps_start_found, fhv.len);
            }
            fprintf(stderr, "Found IDR at %d, len %d\n", idr_start_found, fhi.len);
        }
    } else {
        if (debug) fprintf(stderr, "No frame found\n");
        exit(-8);
    }

    // Add FF_INPUT_BUFFER_PADDING_SIZE to make the size compatible with ffmpeg conversion
    bufferh26x = (unsigned char *) malloc(fhv.len + fhs.len + fhp.len + fhi.len + FF_INPUT_BUFFER_PADDING_SIZE);
    if (bufferh26x == NULL) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(-9);
    }

    bufferyuv = (unsigned char *) malloc(width * height * 3 / 2);
    if (bufferyuv == NULL) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(-10);
    }

    if (fhv_addr != NULL) {
        memcpy(bufferh26x, fhv_addr, fhv.len);
    }
    memcpy(bufferh26x + fhv.len, fhs_addr, fhs.len);
    memcpy(bufferh26x + fhv.len + fhs.len, fhp_addr, fhp.len);
    memcpy(bufferh26x + fhv.len + fhs.len + fhp.len, fhi_addr, fhi.len);

    free(h26x_file_buffer);

    if (fhv_addr == NULL) {
        if (debug) fprintf(stderr, "Decoding h264 frame\n");
        if(frame_decode(bufferyuv, bufferh26x, fhs.len + fhp.len + fhi.len, 4) < 0) {
            fprintf(stderr, "Error decoding h264 frame\n");
            exit(-11);
        }
    } else {
        if (debug) fprintf(stderr, "Decoding h265 frame\n");
        if(frame_decode(bufferyuv, bufferh26x, fhv.len + fhs.len + fhp.len + fhi.len, 5) < 0) {
            fprintf(stderr, "Error decoding h265 frame\n");
            exit(-11);
        }
    }
    free(bufferh26x);

    if (watermark) {
        if (debug) fprintf(stderr, "Adding watermark\n");
        if (add_watermark(bufferyuv, width, height) < 0) {
            fprintf(stderr, "Error adding watermark\n");
            exit(-12);
        }
    }

    if (debug) fprintf(stderr, "Encoding jpeg image\n");
    if(YUVtoJPG("stdout", bufferyuv, width, height, width, height, quality) < 0) {
        fprintf(stderr, "Error encoding jpeg file\n");
        exit(-13);
    }

    free(bufferyuv);

    return 0;
}
