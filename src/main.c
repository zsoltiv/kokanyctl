#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "SDL.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_ttf.h"
#include "SDL_net.h"

#include "net.h"
#include "utils.h"

uint8_t encode_scancode(uint8_t scancode, bool pressed)
{
    return (uint8_t)SDL_GetKeyFromScancode(scancode) | (uint8_t)(pressed << 7);
}

int main(void)
{
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

    IPaddress local;
    net_get_local_address(&local);
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

        SDL_RenderCopy(rend, text, NULL, &textrect);
        
        if(keys[SDL_SCANCODE_Q])
            break;
        // render

        SDL_RenderPresent(rend);
    }

    SDL_Quit();
    SDLNet_Quit();
}
