#ifndef IMGPROC_H
#define IMGPROC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_net.h"

#include "video.h"

struct imgproc_data;

struct imgproc_data *imgproc_init(struct video_data *video_data, IPaddress *video_addr);
void imgproc_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* IMGPROC_H */

