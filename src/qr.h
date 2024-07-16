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

#ifndef QR_H
#define QR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#include "framelist.h"

struct qr;

struct qr *qr_init(struct frame *f,
                   const unsigned int width,
                   const unsigned int height,
                   enum AVPixelFormat pix_fmt,
                   const char *outfilepath);
int qr_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* QR_H */
