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

#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <zbar.h>
#include "SDL.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "framelist.h"
#include "qr.h"

struct qr {
    zbar_image_t *img;
    zbar_processor_t *processor;
    FILE *outfile;
    struct frame *frames;
    uint8_t *buf;
    int bufsize;
};

struct qr *qr_init(struct frame *f,
                   const unsigned int width,
                   const unsigned int height,
                   enum AVPixelFormat pix_fmt)
{
    struct qr *qr = malloc(sizeof(struct qr));
    qr->processor = zbar_processor_create(1);
    qr->frames = f;
    zbar_processor_set_config(qr->processor, 0, ZBAR_CFG_ENABLE, 0);
    zbar_processor_set_config(qr->processor, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);
    qr->img = zbar_image_create();
    qr->outfile = fopen("codes.txt", "w");
    if(!qr->outfile)
        fprintf(stderr, "failed to open codes.txt\n");
    uint32_t fourcc = avcodec_pix_fmt_to_codec_tag(pix_fmt);
    if(!fourcc)
        fprintf(stderr,
                "Pixel format %s (%u) has no FourCC!\n",
                av_get_pix_fmt_name(pix_fmt),
                pix_fmt);
    zbar_image_set_format(qr->img, fourcc);
    zbar_image_set_size(qr->img, width, height);

    return qr;
}

static void qr_copy_frame(struct frame *f, struct qr *qr)
{
    qr->bufsize = av_image_get_buffer_size(f->avf->format,
                                           f->avf->width,
                                           f->avf->height,
                                           1);
    if(qr->bufsize <= 0) {
        fprintf(stderr, "copy gebasz\n");
        return;
    }
    qr->buf = malloc(qr->bufsize);
    if(av_image_copy_to_buffer(qr->buf,
                               qr->bufsize,
                               (const uint8_t *const *)f->avf->data,
                               f->avf->linesize,
                               f->avf->format,
                               f->avf->width,
                               f->avf->height,
                               1) < 0)
        fprintf(stderr, "av_image_copy_to_buffer() failed\n");
}

int qr_thread(void *arg)
{
    struct qr *qr = (struct qr *)arg;
    struct frame *f = qr->frames;
    int ret;
    while(true) {
        while(!((f = frame_list_lock_next(f))->ready)) {
            frame_list_unlock_frame(f, false);
        }
        qr_copy_frame(f, qr);
        frame_list_unlock_frame(f, true);
        if(!qr->buf || qr->bufsize <= 0) continue;

        zbar_image_set_data(qr->img, qr->buf, qr->bufsize, zbar_image_free_data);
        if((ret = zbar_process_image(qr->processor, qr->img)) < 0)
            fprintf(stderr, "zbar_process_image() failed\n");
        
        for(const zbar_symbol_t *sym = zbar_image_first_symbol(qr->img); sym; sym = zbar_symbol_next(sym)) {
            zbar_symbol_type_t type = zbar_symbol_get_type(sym);
            if(type == ZBAR_PARTIAL || type == ZBAR_NONE)
                continue;
            const char *data = zbar_symbol_get_data(sym);
            printf("Decoded data: %s\n", data);
            fprintf(qr->outfile, "%s\n", data);
        }
    }
    return 0;
}
