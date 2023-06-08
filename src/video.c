#include <libavutil/frame.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "SDL.h"

#include "SDL_render.h"
#include "utils.h"
#include "video.h"

/*
 * This thread receives video data over a socket and decodes it
 * into `AVFrame`s for the image processing thread to use
*/

#define REMOTE_OPTS "?listen=1"
// FIXME use kokanybot's static IP
#define REMOTE "tcp://127.0.0.1:1337" REMOTE_OPTS

struct image {
    AVFrame *decoded;
    int pitch;
};

struct av {
    /*
     * libav* stuff
    */
    AVFormatContext *fmt;
    AVCodecContext *decoder;
    AVPacket *pkt;
};

struct video_data {
    struct av av;
    // shared data
    int width, height;
    SDL_mutex *lock;
    SDL_Texture *screen;
    struct image img;
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
                         video_data->img.decoded->data[0],
                         video_data->img.decoded->linesize[0],
                         video_data->img.decoded->data[1],
                         video_data->img.decoded->linesize[1],
                         video_data->img.decoded->data[2],
                         video_data->img.decoded->linesize[2]);
}

struct video_data *video_init(SDL_Renderer *rend)
{
    struct video_data *video = malloc(sizeof(struct video_data));
    struct av *av = &video->av; // alias for simplicity
    avformat_network_init();
    av->fmt = avformat_alloc_context();
    if(avformat_open_input(&av->fmt, REMOTE, NULL, NULL) < 0)
        fprintf(stderr, "avformat_open_input() failed\n");
    fprintf(stderr, "Format: %s\n", av->fmt->iformat->long_name);
    if(avformat_find_stream_info(av->fmt, NULL) < 0)
        fprintf(stderr, "avformat_find_stream_info() failed\n");

    if(av->fmt->nb_streams != 1)
        ctl_die("Too many streams!\n");
    AVStream *stream = av->fmt->streams[0];
    if(stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
        ctl_die("Not a video stream!\n");
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    fprintf(stderr, "Using decoder %s\n", codec->long_name);
    av->decoder = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(av->decoder, stream->codecpar);
    video->width = av->decoder->width;
    video->height = av->decoder->height;
    avcodec_open2(av->decoder, codec, NULL);
    fprintf(stderr, "swscale initialized!\n");

    av->pkt = av_packet_alloc();
    video->img.decoded = av_frame_alloc();
    video->lock = SDL_CreateMutex();

    assert(av->decoder->pix_fmt == AV_PIX_FMT_YUV420P);

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
    AVFrame *restrict decoded = video_data->img.decoded;
    int ret;

    while(1) {
        if((ret = av_read_frame(av->fmt, av->pkt)) < 0)
            fprintf(stderr, "av_read_frame() failed\n");
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
        printf("pitch %d\n", video_data->img.pitch);

        if(ret < 0)
            fprintf(stderr, "av_image_alloc() failed\n");
        printf("Frame decoded!\n");
    }
}
