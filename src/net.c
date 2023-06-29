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

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

#include "SDL.h"

#include "utils.h"
#include "net.h"

#define NET_FFMPEG_PROTO "tcp://"

int net_connect_to_remote(struct sockaddr *remote)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(sock, remote, sizeof(*remote)) < 0)
        perror("connect()");
    return sock;
}

struct sockaddr net_resolve_host(const char *remote, const char *port)
{
    printf("%s:%s\n", remote, port);
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    struct addrinfo *res;
    int ret;
    if((ret = getaddrinfo(remote, port, &hints, &res)))
        ctl_die("getaddrinfo(): %s", gai_strerror(ret));
    struct sockaddr out;
    memcpy(&out, res->ai_addr, sizeof(struct sockaddr));
    freeaddrinfo(res);

    char buf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &((struct sockaddr_in *)&out)->sin_addr, buf, sizeof(buf));
    printf("Resolved %s to %s\n", remote, buf);
    printf("%s port is %u\n", buf, ntohs(((struct sockaddr_in *)&out)->sin_port));

    return out;
}

void net_send_keycode(int remote, uint8_t keycode)
{
    if(send(remote, &keycode, 1, 0) < 0)
        perror("send()");
}

uint8_t net_encode_scancode(uint8_t scancode, bool pressed)
{
    uint8_t pressed_u8 = pressed << 7;
    assert(pressed_u8 == 0x80 || pressed_u8 == 0);
    return (uint8_t)SDL_GetKeyFromScancode(scancode) | pressed_u8;
}

const char *net_ffmpeg_format_url(const char *ip_string, const char *port)
{
    size_t port_sz = strlen(port);
    size_t ip_sz = strlen(ip_string);
    size_t ffmpeg_proto_sz = strlen(NET_FFMPEG_PROTO);
    size_t total = ffmpeg_proto_sz + ip_sz + port_sz + 2;
    char *buf = calloc(total + 1, 1);
    snprintf(buf,
             total,
             "%s%s:%s",
             NET_FFMPEG_PROTO,
             ip_string,
             port);
    printf("URL %s\n", buf);

    return buf;
}
