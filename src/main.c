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

#include <asm-generic/errno-base.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>

#include "SDL.h"
#include "SDL_ttf.h"

#include "net.h"
#include "video.h"
#include "utils.h"

#define PORT_CTL "1337"
#define PORT_VIDEO "1338"
#define PORT_SENSOR "1339"

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
};
bool prev_keys[UINT8_MAX] = {0};

int main(int argc, char *argv[])
{
    if(argc != 2)
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

    const char *stream_uri = net_ffmpeg_format_url(argv[1], PORT_VIDEO);
    struct video_data *video_data = video_init(rend, stream_uri);
    SDL_CreateThread(video_thread, "video", video_data);
    int remote = net_connect_to_remote(argv[1], PORT_CTL);
    int sensor = net_connect_to_remote(argv[1], PORT_SENSOR);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);

    int key_count;
    const uint8_t *keys = SDL_GetKeyboardState(&key_count);

    SDL_Surface *present_surface = TTF_RenderUTF8_Solid(font, "Gas present", (SDL_Color) {255, 0, 0, 255});
    SDL_Surface *not_present_surface = TTF_RenderUTF8_Solid(font, "No gas present", (SDL_Color) {0, 255, 0, 255});
    SDL_Texture *present = SDL_CreateTextureFromSurface(rend, present_surface);
    SDL_Texture *not_present = SDL_CreateTextureFromSurface(rend, not_present_surface);
    SDL_FreeSurface(present_surface);
    SDL_FreeSurface(not_present_surface);

    const SDL_Rect textrect = {0, 0, 300, 64};

    while(true) {
        SDL_PumpEvents();

        SDL_RenderClear(rend);
        video_lock(video_data);
        video_update_screen(video_data);
        SDL_RenderCopy(rend, video_get_screen(video_data), NULL, NULL);
        video_unlock(video_data);
        bool co2_present;
        if(recv(sensor, &co2_present, sizeof(bool), 0) < 0) {
            if(errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv()");
                exit(1);
            }
        }
        SDL_RenderCopy(rend, co2_present ? present : not_present, NULL, &textrect);

        for(int i = 0; i < sizeof(handled_scancodes); i++) {
            if(keys[handled_scancodes[i]] != prev_keys[handled_scancodes[i]]) {
                net_send_keycode(remote, net_encode_scancode(handled_scancodes[i], keys[handled_scancodes[i]]));
                prev_keys[handled_scancodes[i]] = keys[handled_scancodes[i]];
            }
        }
        if(keys[SDL_SCANCODE_Q])
            break;

        SDL_RenderPresent(rend);
    }

    SDL_Quit();
}
