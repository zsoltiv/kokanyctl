#ifndef FRAMELIST_H
#define FRAMELIST_H

#include <libavutil/frame.h>
#include "SDL.h"

#include <stdbool.h>

struct frame {
    AVFrame *avf;
    SDL_mutex *lock;
    struct frame *next;
    bool ready; // consumers check this before using `avf`
};

struct frame *frame_list_new(const int elems);
struct frame *frame_list_lock_next(struct frame *current);
void frame_list_unlock_frame(struct frame *f, bool isready);
void frame_print(struct frame *f);

#endif
