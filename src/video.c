/*
 * copyright (c) 2023 Zsolt Vadasz
 *
 * This file is part of kokanyctl.
 *
 * kokanyctl is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * kokanyctl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kokanyctl. If not, see <https://www.gnu.org/licenses/>. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include "SDL.h"

#include "qr.h"
#include "framelist.h"
#include "utils.h"
#include "video.h"

/*
 * This thread receives video data
 * over a socket and decodes them for use with SDL2
*/

#define NFRAMES 32

struct av {
    /*
     * internal stuff
    */
    AVFormatContext *fmt;
    AVCodecContext *decoder;
    int video_idx;
    AVPacket *pkt;
    struct qr *qr;
};

struct video_data {
    struct av av;
    struct frame *frames, *current;
    // shared data
    int width, height;
    SDL_mutex *lock;
    SDL_Texture *screen;
    unsigned framenum;
    bool has_frame;
};

SDL_Texture *video_get_screen(const struct video_data *video_data)
{
    return video_data->screen;
}

bool video_has_frame(struct video_data *video)
{
    return video->has_frame;
}

// must be called from main thread
void video_update_screen(struct video_data *video_data)
{
    struct frame *f = video_data->frames;
    int counter = 0;
    while(!((f = frame_list_lock_next(f))->ready)) {
        frame_list_unlock_frame(f, false);
        if(counter == NFRAMES) {
            video_data->has_frame = false;
            return;
        }
        counter++;
    }
    SDL_UpdateYUVTexture(video_data->screen, NULL,
                         f->avf->data[0],
                         f->avf->linesize[0],
                         f->avf->data[1],
                         f->avf->linesize[1],
                         f->avf->data[2],
                         f->avf->linesize[2]);
    video_data->has_frame = true;
    frame_list_unlock_frame(f, false);
}

struct video_data *video_init(SDL_Renderer *rend,
                              const char *restrict uri,
                              const struct camera_data *camera_data)
{
    struct video_data *video = malloc(sizeof(struct video_data));
    struct av *av = &video->av; // alias for simplicity
    int ret;
    avformat_network_init();
    av->fmt = avformat_alloc_context();
    AVDictionary *in_opts = NULL;
    if((ret = av_dict_set(&in_opts,
                          "fflags",
                          "nobuffer",
                          0)) < 0 ||
       (ret = av_dict_set(&in_opts,
                          "overrun_nonfatal",
                          "1",
                          0)) < 0 ||
       (ret = av_dict_set(&in_opts,
                          "fifo_size",
                          "50000000", // 4096 * 10
                          0)) < 0 ||
       (ret = av_dict_set(&in_opts,
                          "reuse",
                          "1",
                          0)) < 0 ||
       (ret = av_dict_set(&in_opts,
                          "timeout",
                          "500000",
                          0)) < 0) {
        fprintf(stderr, "av_dict_set(): %s\n", av_err2str(ret));
        return NULL;
    }

    // automate reconnecting when AVERROR_INVALIDDATA is returned at the start of a stream
    do {
        ret = avformat_open_input(&av->fmt, uri, NULL, &in_opts);
        if(ret < 0) {
            fprintf(stderr, "avformat_open_input() failed: %s\n", av_err2str(ret));
            if(ret != AVERROR_INVALIDDATA)
                return NULL;
        }
    } while(ret == AVERROR_INVALIDDATA);

    fprintf(stderr, "Format: %s\n", av->fmt->iformat->long_name);
    if(avformat_find_stream_info(av->fmt, NULL) < 0) {
        fprintf(stderr, "avformat_find_stream_info() failed\n");
        return NULL;
    }

    for(unsigned i = 0; i < av->fmt->nb_streams; i++) {
        printf("Stream #%u: %s\n", i, avcodec_get_name(av->fmt->streams[i]->codecpar->codec_id));
        if(av->fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            av->video_idx = i;
    }
    AVStream *stream = av->fmt->streams[av->video_idx];
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    fprintf(stderr, "Using decoder %s\n", codec->long_name);
    av->decoder = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(av->decoder, stream->codecpar);
    video->width = av->decoder->width;
    video->height = av->decoder->height;
    avcodec_open2(av->decoder, codec, NULL);

    av->pkt = av_packet_alloc();
    video->frames = frame_list_new(NFRAMES);
    video->current = video->frames;
    video->framenum = 0;
    if(camera_data->process_qr_codes) {
        av->qr = qr_init(video->frames,
                         video->width,
                         video->height,
                         av->decoder->pix_fmt,
                         camera_data->qr_file);
        SDL_CreateThread(qr_thread, "qr", av->qr);
        printf("QR thread initialised\n");
    } else {
        av->qr = NULL;
    }

    printf("Pixel format: %s\n", av_get_pix_fmt_name(av->decoder->pix_fmt));

    video->screen = SDL_CreateTexture(rend,
                                      camera_data->pix_fmt,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      video->width,
                                      video->height);
    if(!video->screen)
        ctl_die("%s\n", SDL_GetError());

    video->has_frame = false;
    //av_log_set_level(AV_LOG_PANIC);

    return video;
}

int video_thread(void *arg)
{
    struct video_data *video_data = (struct video_data *)arg;
    struct frame *current = video_data->current;
    struct av *av = &video_data->av;
    int ret;

    while(1) {
        if((ret = av_read_frame(av->fmt, av->pkt)) < 0) {
            fprintf(stderr, "av_read_frame() failed: %s\n", av_err2str(ret));
            continue;
        }
        if((ret = avcodec_send_packet(av->decoder, av->pkt)) < 0)
            fprintf(stderr, "avcodec_send_packet() failed: %s\n", av_err2str(ret));
        while((current = frame_list_lock_next(current))->ready)
            frame_list_unlock_frame(current, true);
        if((ret = avcodec_receive_frame(av->decoder, current->avf)) < 0) {
            frame_list_unlock_frame(current, false);
            if(ret == AVERROR(EAGAIN)) {
                continue;
            }
            fprintf(stderr, "avcodec_receive_frame() failed: %s\n", av_err2str(ret));
        }
        frame_list_unlock_frame(current, true);
        video_data->framenum++;
    }

    return ret;
}
