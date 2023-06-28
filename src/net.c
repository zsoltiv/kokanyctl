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

#include <ifaddrs.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

#include "SDL_net.h"

#include "utils.h"

#include "net.h"

#define NET_FFMPEG_PROTO "tcp://"

TCPsocket net_connect_to_remote(IPaddress *remote)
{
    printf("%x:%u\n", remote->host, remote->port);
    TCPsocket sock =  SDLNet_TCP_Open(remote);
    if(!sock)
        ctl_die("SDLNet_TCP_Open(): %s\n", SDLNet_GetError());

    return sock;
}

IPaddress net_resolve_host(const char *remote, uint16_t port)
{
    IPaddress ip;
    SDLNet_ResolveHost(&ip, remote, htons(port));
    return ip;
}

void net_send_keycode(TCPsocket remote, uint8_t keycode)
{
    if(SDLNet_TCP_Send(remote,
                       &keycode,
                       1) < 1)
        ctl_die("SDLNet_TCP_Send(): %s", SDLNet_GetError());

}

uint8_t net_encode_scancode(uint8_t scancode, bool pressed)
{
    uint8_t pressed_u8 = pressed << 7;
    assert(pressed_u8 == 0x80 || pressed_u8 == 0);
    return (uint8_t)SDL_GetKeyFromScancode(scancode) | pressed_u8;
}

const char *net_ffmpeg_format_url(IPaddress *ip)
{
    const char *ip_string = SDLNet_ResolveIP(ip);
    if(!ip_string)
        return NULL;

    char port[13] = {0};
    snprintf(port, sizeof(port), ":%u", ip->port);

    size_t port_sz = strlen(port);
    size_t ip_sz = strlen(ip_string);
    size_t ffmpeg_proto_sz = strlen(NET_FFMPEG_PROTO);
    size_t total = ffmpeg_proto_sz + ip_sz + port_sz + 1;
    char *buf = calloc(total + 1, 1);
    snprintf(buf,
             total,
             "%s%s%s",
             NET_FFMPEG_PROTO,
             ip_string,
             port);
    printf("URL %s\n", buf);

    return buf;
}
