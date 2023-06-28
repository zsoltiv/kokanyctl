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
#include <zbar.h>
#include "SDL.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "SDL_mutex.h"
#include "qr.h"

struct qr {
    zbar_image_t *img;
    zbar_processor_t *processor;
    SDL_mutex *lock;
    SDL_cond *cond;
    uint8_t *buf;
    int bufsize;
};

struct qr *qr_init(const unsigned int width,
                   const unsigned int height,
                   const enum AVPixelFormat pix_fmt)
{
    struct qr *qr = malloc(sizeof(struct qr));
    qr->lock = SDL_CreateMutex();
    qr->cond = SDL_CreateCond();
    qr->processor = zbar_processor_create(0);
    qr->img = zbar_image_create();
    qr->bufsize = 0;
    zbar_image_set_format(qr->img, zbar_fourcc('Y', 'U', '1', '2'));
    zbar_image_set_size(qr->img, width, height);
    qr->buf = NULL;

    return qr;
}

void qr_send_frame(struct qr *qr, AVFrame *frame)
{
    SDL_LockMutex(qr->lock);
    qr->bufsize = av_image_get_buffer_size(frame->format,
                                           frame->width,
                                           frame->height,
                                           1);
    if(qr->bufsize < 0)
        fprintf(stderr, "av_image_get_buffer_size() failed\n");
    qr->buf = malloc(qr->bufsize);
    if(av_image_copy_to_buffer(qr->buf,
                               qr->bufsize,
                               (const uint8_t *const *)frame->data,
                               frame->linesize,
                               frame->format,
                               frame->width,
                               frame->height,
                               1) < 0)
        fprintf(stderr, "av_image_copy_to_buffer() failed\n");
    SDL_UnlockMutex(qr->lock);
    SDL_CondSignal(qr->cond);
}

int qr_thread(void *arg)
{
    struct qr *qr = (struct qr *)arg;
    int ret;
    while(true) {
        SDL_LockMutex(qr->lock);
        SDL_CondWait(qr->cond, qr->lock);

        if(qr->buf)
            zbar_image_set_data(qr->img, qr->buf, qr->bufsize, zbar_image_free_data);
        if((ret = zbar_process_image(qr->processor, qr->img)) < 0) {
            fprintf(stderr, "zbar_process_image() failed\n");
        }
        if(!ret)
            fprintf(stderr, "No QR codes detected\n");
        
        for(const zbar_symbol_t *sym = zbar_image_first_symbol(qr->img); sym; sym = zbar_symbol_next(sym)) {
            zbar_symbol_type_t type = zbar_symbol_get_type(sym);
            printf("QR type: %s\n", zbar_get_symbol_name(type));
            unsigned sym_len = zbar_symbol_get_data_length(sym);
            if(type == ZBAR_PARTIAL || type == ZBAR_NONE)
                continue;
            printf("Decoded data: %s\n", zbar_symbol_get_data(sym));
        }

        SDL_UnlockMutex(qr->lock);
    }
    return 0;
}
