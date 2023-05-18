#ifndef NET_H
#define NET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "SDL_net.h"

IPaddress net_resolve_host(const char *remote, uint16_t port);
TCPsocket net_connect_to_remote(IPaddress *remote);
uint8_t net_encode_scancode(uint8_t scancode, bool pressed);
void net_send_keycode(TCPsocket remote, uint8_t keycode);
const char *net_ffmpeg_format_url(IPaddress *ip);

#ifdef __cplusplus
}
#endif

#endif /* NET_H */
