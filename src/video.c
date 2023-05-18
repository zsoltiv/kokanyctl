#include <stdlib.h>
#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "SDL.h"

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
    int references;
    int linesizes[4];
    /*
     * must be an array of 4 because of `av_image_alloc`'s prototype
     * only the first pointer is used
    */
    uint8_t *bgr[4];
};

struct av {
    /*
     * libav* stuff
    */
    AVFormatContext *fmt;
    AVCodecContext *decoder;
    AVPacket *pkt;
    struct SwsContext *sws;
};

struct video_data {
    struct av av;
    // shared data
    int width, height;
    SDL_mutex *lock;
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

struct video_data *video_init(void)
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
    av->sws = sws_getCachedContext(av->sws,
                               av->decoder->width,
                               av->decoder->height,
                               av->decoder->pix_fmt,
                               video->width,
                               video->height,
                               AV_PIX_FMT_BGR24,
                               SWS_ACCURATE_RND | SWS_FULL_CHR_H_INT,
                               NULL,
                               NULL,
                               NULL);
    fprintf(stderr, "swscale initialized!\n");

    av->pkt = av_packet_alloc();
    video->lock = SDL_CreateMutex();

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

    AVFrame *decoded = av_frame_alloc();
    int ret;

    while(1) {
        if((ret = av_read_frame(av->fmt, av->pkt)) < 0)
            fprintf(stderr, "av_read_frame() failed\n");
        if((ret = avcodec_send_packet(av->decoder, av->pkt)) < 0)
            fprintf(stderr, "avcodec_send_packet() failed\n");
        if((ret = avcodec_receive_frame(av->decoder, decoded)) < 0) {
            if(ret == AVERROR(EAGAIN))
                continue;
            fprintf(stderr, "avcodec_receive_frame() failed\n");
        }
        printf("width: %d height: %d\n", decoded->width, decoded->height);
        ret = av_image_alloc(video_data->img.bgr,
                             video_data->img.linesizes,
                             video_data->width,
                             video_data->height,
                             AV_PIX_FMT_BGR24,
                             1);
        if(ret < 0)
            fprintf(stderr, "av_image_alloc() failed\n");
        sws_scale(av->sws,
                  (const uint8_t *const *)decoded->data,
                  decoded->linesize,
                  0,
                  decoded->height,
                  video_data->img.bgr,
                  video_data->img.linesizes);
        printf("Frame decoded!\n");
        // just free it for now
        av_freep(video_data->img.bgr);
    }
}
