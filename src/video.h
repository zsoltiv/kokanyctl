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

#ifndef VIDEO_H
#define VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL.h"

struct video_data;

struct video_data *video_init(SDL_Renderer *rend,
                              const char *restrict uri,
                              SDL_PixelFormatEnum sdl_pix_fmt);
SDL_Texture *video_get_screen(const struct video_data *video_data);
void video_update_screen(struct video_data *video_data);
void video_scan_qr(struct video_data *video);
int video_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif
