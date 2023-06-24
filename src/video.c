#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include "SDL.h"

#include "SDL_render.h"
#include "utils.h"
#include "video.h"

/*
 * This thread receives video and audio data
 * over a socket and decodes them for use with SDL2
*/

struct av {
    /*
     * libav* stuff
    */
    AVFormatContext *fmt;
    AVCodecContext *decoder;
    int audio_idx, video_idx;
    AVPacket *pkt;
};

struct video_data {
    struct av av;
    AVFrame *decoded;
    // shared data
    int width, height;
    SDL_mutex *lock;
    SDL_Texture *screen;
};

int video_get_width(struct video_data *video_data)
{
    return video_data->width;
}

int video_get_height(struct video_data *video_data)
{
    return video_data->height;
}

SDL_Texture *video_get_screen(struct video_data *video_data)
{
    return video_data->screen;
}

// must be called from main thread
void video_update_screen(struct video_data *video_data)
{
    SDL_UpdateYUVTexture(video_data->screen, NULL,
                         video_data->decoded->data[0],
                         video_data->decoded->linesize[0],
                         video_data->decoded->data[1],
                         video_data->decoded->linesize[1],
                         video_data->decoded->data[2],
                         video_data->decoded->linesize[2]);
}

struct video_data *video_init(SDL_Renderer *rend, const char *restrict uri)
{
    struct video_data *video = malloc(sizeof(struct video_data));
    struct av *av = &video->av; // alias for simplicity
    int ret;
    avformat_network_init();
    av->fmt = avformat_alloc_context();
    AVInputFormat *mpegts = av_find_input_format("mpegts");
    if(!mpegts)
        fprintf(stderr, "mpegts not supported\n");
    if((ret = avformat_open_input(&av->fmt, uri, mpegts, NULL)) < 0) {
        fprintf(stderr, "avformat_open_input() failed: %s\n", av_err2str(ret));
    }
    fprintf(stderr, "Format: %s\n", av->fmt->iformat->long_name);
    if(avformat_find_stream_info(av->fmt, NULL) < 0)
        fprintf(stderr, "avformat_find_stream_info() failed\n");

    for(int i = 0; i < av->fmt->nb_streams; i++) {
        printf("Stream #%d: %s\n", i, avcodec_get_name(av->fmt->streams[i]->codecpar->codec_id));
        if(av->fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            av->video_idx = i;
        else
            av->audio_idx = i;
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
    video->decoded = av_frame_alloc();
    video->lock = SDL_CreateMutex();

    printf("Pixel format: %s\n", av_get_pix_fmt_name(av->decoder->pix_fmt));

    video->screen = SDL_CreateTexture(rend,
                                      SDL_PIXELFORMAT_IYUV,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      video->width,
                                      video->height);
    if(!video->screen)
        ctl_die("%s\n", SDL_GetError());

    return video;
}

void video_lock(struct video_data *video_data)
{
    SDL_LockMutex(video_data->lock);
}
void video_unlock(struct video_data *video_data)
{
    SDL_UnlockMutex(video_data->lock);
}

int video_thread(void *arg)
{
    struct video_data *video_data = (struct video_data *)arg;
    struct av *av = &video_data->av;
    AVFrame *restrict decoded = video_data->decoded;
    int ret;

    // discard all frames received until now
    while(ret != AVERROR(EAGAIN)) {
        if((ret = av_read_frame(av->fmt, av->pkt)) < 0)
            continue;
        if((ret = avcodec_send_packet(av->decoder, av->pkt)) < 0)
            continue;
        ret = avcodec_receive_frame(av->decoder, decoded);
    }

    while(1) {
        if((ret = av_read_frame(av->fmt, av->pkt)) < 0)
            fprintf(stderr, "av_read_frame() failed\n");
        if(av->pkt->stream_index == av->audio_idx)
            continue;
        if((ret = avcodec_send_packet(av->decoder, av->pkt)) < 0)
            fprintf(stderr, "avcodec_send_packet() failed\n");
        video_lock(video_data);
        if((ret = avcodec_receive_frame(av->decoder, decoded)) < 0) {
            video_unlock(video_data);
            if(ret == AVERROR(EAGAIN)) {
                continue;
            }
            fprintf(stderr, "avcodec_receive_frame() failed\n");
        }
        video_unlock(video_data);
        printf("width: %d height: %d\n", decoded->width, decoded->height);

        if(ret < 0)
            fprintf(stderr, "av_image_alloc() failed\n");
        printf("Frame decoded!\n");
    }
}
