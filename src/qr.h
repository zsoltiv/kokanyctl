#ifndef QR_H
#define QR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

struct qr;

struct qr *qr_init(const unsigned int width,
                   const unsigned int height,
                   const enum AVPixelFormat pix_fmt);
void qr_send_frame(struct qr *qr, AVFrame *frame);
int qr_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* QR_H */
