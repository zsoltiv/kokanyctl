#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdbool.h>

#include "SDL_net.h"

void net_get_local_address(IPaddress *ip);
void net_resolve_kokanybot(IPaddress *ip);
uint8_t net_encode_scancode(uint8_t scancode, bool pressed);
const char *net_ffmpeg_format_url(IPaddress *ip);

#endif /* NET_H */
