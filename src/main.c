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

#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>

#include "SDL.h"
#include "SDL_events.h"
#include "SDL_ttf.h"

#include "net.h"
#include "video.h"
#include "utils.h"
#include "camera_data.h"

#define PORT_CTL "1337"
#define PORT_SENSOR "1339"

const struct camera_data camera_datas[] = {
    { "1338", SDL_PIXELFORMAT_IYUV, true },
    { "1341", SDL_PIXELFORMAT_IYUV, false },
};

const uint8_t handled_scancodes[] = {
    SDL_SCANCODE_W,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_E,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6,
    SDL_SCANCODE_7,
    SDL_SCANCODE_8,
};

int main(int argc, char *argv[])
{
    if(argc != 3)
        return 1;
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0)
        ctl_die("SDL init error: %s\n", SDL_GetError());
    if(TTF_Init() < 0)
        ctl_die("TTF init error: %s\n", TTF_GetError());

    const char *ttf_path = "ttf/undefined-medium.ttf";
    TTF_Font *font = TTF_OpenFont(ttf_path, 32);
    if(!font)
        ctl_die("TTF error: failed to open %s: %s\n", ttf_path, TTF_GetError());
    SDL_Window *win = SDL_CreateWindow("kokanyctl",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       1280,
                                       720,
                                       SDL_WINDOW_RESIZABLE);
    if(!win)
        ctl_die("SDL window creation error: %s\n", SDL_GetError());
    SDL_Renderer *rend = SDL_CreateRenderer(win,
                                            -1,
                                            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!rend)
        ctl_die("SDL renderer creation error: %s\n", SDL_GetError());

    unsigned camera_data_idx = (unsigned)strtoul(argv[2], NULL, 10);
    if(camera_data_idx >= sizeof(camera_datas) / sizeof(camera_datas[0])) {
        fprintf(stderr,
                "%u is not a valid camera index! There are only %zu cameras\n",
                camera_data_idx,
                sizeof(camera_datas) / sizeof(camera_datas[0]));
        return 1;
    }

    const char *stream_uri = net_ffmpeg_format_url("udp",
                                                   "127.0.0.1",
                                                   camera_datas[camera_data_idx].portstr);
    if(!stream_uri) {
        fprintf(stderr, "XDG_RUNTIME_DIR must be set\n");
        return 1;
    }
    struct video_data *video_data = video_init(rend,
                                               stream_uri,
                                               &camera_datas[camera_data_idx]);
    if(video_data) {
        SDL_CreateThread(video_thread, "video", video_data);
    }
    struct sockaddr remote_addr;
    int remote = net_udp_socket(argv[1], PORT_CTL, &remote_addr);
    //int sensor = net_connect_to_remote(argv[1], PORT_SENSOR);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);

    bool quit = false;
    SDL_Event ev;
    while(!quit) {
        while(SDL_PollEvent(&ev)) {
            switch(ev.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYUP:
                    for(size_t i = 0; i < sizeof(handled_scancodes); i++)
                        if(ev.key.keysym.scancode == handled_scancodes[i]) {
                            net_send_keycode(remote, net_encode_scancode(ev.key.keysym.scancode, false), &remote_addr);
                            break;
                        }
                    break;
                case SDL_KEYDOWN:
                    puts("aa");
                    for(size_t i = 0; i < sizeof(handled_scancodes); i++)
                        if(ev.key.keysym.scancode == handled_scancodes[i]) {
                            net_send_keycode(remote, net_encode_scancode(ev.key.keysym.scancode, true), &remote_addr);
                            break;
                        }
                    break;
            }
        }

        SDL_RenderClear(rend);
        if(video_data) {
            video_update_screen(video_data);
            SDL_RenderCopy(rend, video_get_screen(video_data), NULL, NULL);
        }
        SDL_RenderPresent(rend);
    }

    SDL_Quit();
}
