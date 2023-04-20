#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdbool.h>

#include "SDL_net.h"

TCPsocket net_connect_to_remote(void);
uint8_t net_encode_scancode(uint8_t scancode, bool pressed);
void net_send_keycode(TCPsocket remote, uint8_t keycode);
const char *net_ffmpeg_format_url(IPaddress *ip);

#endif /* NET_H */
