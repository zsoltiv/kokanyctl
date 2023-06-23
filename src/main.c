#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_net.h"

#include "net.h"
#include "video.h"
#include "utils.h"

#define PORT_CTL 1337
#define PORT_VIDEO 1338

static const uint8_t handled_scancodes[] = {
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

int main(int argc, char *argv[])
{
    if(argc != 2)
        return 1;
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
        ctl_die("SDL init error: %s\n", SDL_GetError());
    if(SDLNet_Init() < 0)
        ctl_die("SDLNet init error: %s\n", SDLNet_GetError());
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
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if(!rend)
        ctl_die("SDL renderer creation error: %s\n", SDL_GetError());

    IPaddress ctl_addr = net_resolve_host(argv[1], PORT_CTL);
    IPaddress video_addr = net_resolve_host(argv[1], PORT_VIDEO);
    const char *stream_uri = net_ffmpeg_format_url(&video_addr);
    struct video_data *video_data = video_init(rend, stream_uri);
    SDL_CreateThread(video_thread, "video", video_data);
    TCPsocket remote = net_connect_to_remote(&ctl_addr);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);

    int key_count;
    const uint8_t *keys = SDL_GetKeyboardState(&key_count);

    SDL_Surface *textsurf = TTF_RenderUTF8_Solid(font, "kokanyctl initialized", (SDL_Color) {.r = 255, .g = 255, .b = 255, .a = 255});
    SDL_Texture *text = SDL_CreateTextureFromSurface(rend, textsurf);
    SDL_FreeSurface(textsurf);

    const SDL_Rect textrect = {0, 0, 200, 32};

    while(true) {
        SDL_PumpEvents();

        SDL_RenderClear(rend);
        video_lock(video_data);
        video_update_screen(video_data);
        SDL_RenderCopy(rend, video_get_screen(video_data), NULL, NULL);
        video_unlock(video_data);
        SDL_RenderCopy(rend, text, NULL, &textrect);
        
        for(int i = 0; i < sizeof(handled_scancodes); i++) {
            if(keys[handled_scancodes[i]])
                net_send_keycode(remote, net_encode_scancode(handled_scancodes[i], true));
            else
                net_send_keycode(remote, net_encode_scancode(handled_scancodes[i], false));
        }
        if(keys[SDL_SCANCODE_Q])
            break;

        SDL_RenderPresent(rend);
    }

    SDL_Quit();
    SDLNet_Quit();
}
