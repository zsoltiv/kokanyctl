#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_net.h"

void ctl_die(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
    exit(1);
}

uint8_t encode_scancode(uint8_t scancode, bool pressed)
{
    return (uint8_t)SDL_GetKeyFromScancode(scancode) | (uint8_t)(pressed << 7);
}

int main(void)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
        ctl_die("SDL error: %s\n", SDL_GetError());
    if(SDLNet_Init() < 0)
        ctl_die("SDLNet error: %s\n", SDLNet_GetError());
    SDL_Window *win = SDL_CreateWindow("kokanyctl",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       1280,
                                       720,
                                       SDL_WINDOW_RESIZABLE);
    if(!win)
        ctl_die("SDL error: %s\n", SDL_GetError());
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if(!rend)
        ctl_die("SDL error: %s\n", SDL_GetError());

    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);

    int key_count;
    const uint8_t *keys = SDL_GetKeyboardState(&key_count);

    while(true) {
        SDL_PumpEvents();

        SDL_RenderClear(rend);
        
        if(keys[SDL_SCANCODE_Q])
            break;
        // render

        SDL_RenderPresent(rend);
    }

    SDL_Quit();
    SDLNet_Quit();
}
