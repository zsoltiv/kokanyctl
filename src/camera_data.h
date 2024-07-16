#ifndef CAMERA_DATA_H
#define CAMERA_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "SDL.h"

struct camera_data {
    const char *const portstr;
    const char *qr_file;
    SDL_PixelFormatEnum pix_fmt;
    bool process_qr_codes;
};

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_DATA_H */
