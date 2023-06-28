/*
 * copyright (c) 2023 Zsolt Vadasz
 *
 * This file is part of kokanyctl.
 *
 * kokanyctl is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * kokanyctl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kokanyctl. If not, see <https://www.gnu.org/licenses/>. 
*/

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
