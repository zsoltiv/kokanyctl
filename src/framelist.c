#include <libavutil/frame.h>
#include "SDL.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "SDL_mutex.h"
#include "framelist.h"

static struct frame *frame_alloc(void)
{
    struct frame *f = malloc(sizeof(struct frame));
    f->avf = av_frame_alloc();
    f->lock = SDL_CreateMutex();
    f->ready = false;
    f->next = NULL;
    return f;
}

struct frame *frame_list_new(const int elems)
{
    assert(elems > 0);
    struct frame *head = frame_alloc();
    struct frame *p = head;
    for(int i = 1; i < elems; i++) {
        p->next = frame_alloc();
        p = p->next;
    }
    p->next = head;

    return head;
}

struct frame *frame_list_lock_next(struct frame *current)
{
    struct frame *p = current;
    do {
        p = p->next;
    } while(SDL_TryLockMutex(p->lock) == SDL_MUTEX_TIMEDOUT);
    return p;
}

void frame_list_unlock_frame(struct frame *f, bool isready)
{
    f->ready = isready;
    SDL_UnlockMutex(f->lock);
}
