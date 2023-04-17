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
    int width, height, references;
    int linesizes[4];
    /*
     * must be an array of 4 because of `av_image_alloc`'s prototype
     * only the first pointer is used
    */
    uint8_t *bgr[4];
};

struct video_data {
    // shared data
    SDL_mutex *lock;
    struct image img;
};

static AVFormatContext *fmt;
static AVCodecContext *decoder;
static AVPacket *pkt;
static struct SwsContext *sws = NULL;

struct video_data *video_init(void)
{
    avformat_network_init();
    fmt = avformat_alloc_context();
    if(avformat_open_input(&fmt, REMOTE, NULL, NULL) < 0)
        fprintf(stderr, "avformat_open_input() failed\n");
    fprintf(stderr, "Format: %s\n", fmt->iformat->long_name);
    if(avformat_find_stream_info(fmt, NULL) < 0)
        fprintf(stderr, "avformat_find_stream_info() failed\n");

    if(fmt->nb_streams != 1)
        ctl_die("Too many streams!\n");
    AVStream *stream = fmt->streams[0];
    if(stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
        ctl_die("Not a video stream!\n");
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    fprintf(stderr, "Using decoder %s\n", codec->long_name);
    decoder = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(decoder, stream->codecpar);
    avcodec_open2(decoder, codec, NULL);

    pkt = av_packet_alloc();
    struct video_data *video_data = malloc(sizeof(struct video_data));
    video_data->lock = SDL_CreateMutex();

    return video_data;
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
    AVFrame *decoded = av_frame_alloc();
    int ret;

    while(1) {
        if((ret = av_read_frame(fmt, pkt)) < 0)
            fprintf(stderr, "av_read_frame() failed\n");
        if((ret = avcodec_send_packet(decoder, pkt)) < 0)
            fprintf(stderr, "avcodec_send_packet() failed\n");
        if((ret = avcodec_receive_frame(decoder, decoded)) < 0) {
            if(ret == AVERROR(EAGAIN))
                continue;
            fprintf(stderr, "avcodec_receive_frame() failed\n");
        }
        printf("width: %d height: %d\n", decoded->width, decoded->height);
        if(!sws) {
            sws = sws_getCachedContext(sws,
                                       decoded->width,
                                       decoded->height,
                                       decoder->pix_fmt,
                                       decoded->width,
                                       decoded->height,
                                       AV_PIX_FMT_BGR24,
                                       SWS_ACCURATE_RND | SWS_FULL_CHR_H_INT,
                                       NULL,
                                       NULL,
                                       NULL);
            fprintf(stderr, "swscale initialized!\n");
        }
        video_data->img.width = decoded->width;
        video_data->img.height = decoded->height;
        ret = av_image_alloc(video_data->img.bgr,
                             video_data->img.linesizes,
                             video_data->img.width,
                             video_data->img.height,
                             AV_PIX_FMT_BGR24,
                             1);
        if(ret < 0)
            fprintf(stderr, "av_image_alloc() failed\n");
        sws_scale(sws,
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
