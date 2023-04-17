#ifndef VIDEO_H
#define VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

// FIXME set to the same as in kokanybot
#define FRAMES_PER_SEC 24

struct video_data;

struct video_data *video_init(void);
void video_lock(struct video_data *video_data);
void video_unlock(struct video_data *video_data);
int video_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif
