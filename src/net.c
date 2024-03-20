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
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

#include "SDL.h"

#include "utils.h"
#include "net.h"

static void net_resolve_host(struct addrinfo **ai,
                                        const char *remote,
                                        const char *port)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
    };
    int ret;
    if((ret = getaddrinfo(remote, port, &hints, ai)))
        ctl_die("getaddrinfo(): %s", gai_strerror(ret));
    // free everything other than the first one
    if((*ai)->ai_next)
        freeaddrinfo((*ai)->ai_next);
}

void net_get_sockaddr(const char *remote, const char *port, struct sockaddr *out)
{
    struct addrinfo *ai;
    net_resolve_host(&ai, remote, port);
    memcpy(out, ai->ai_addr, sizeof(struct sockaddr));
    freeaddrinfo(ai);
}

int net_connect_to_remote(const char *remote, const char *port)
{
    struct addrinfo *ai;
    net_resolve_host(&ai, remote, port);
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(sock < 0) {
        perror("socket()");
        exit(1);
    }
    struct timespec timeout = {
        .tv_sec = 0,
        // XXX SO_RCVTIMEO actually treats this as `tv_usec`
        .tv_nsec = 10000,
    };
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt()");
        exit(1);
    }
    if(connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
        perror("connect()");
        exit(1);
    }
    printf("connected!\n");
    freeaddrinfo(ai);
    return sock;
}

int net_udp_socket(const char *remote, const char *port, struct sockaddr *out)
{
    struct addrinfo *ai;
    net_resolve_host(&ai, remote, port);
    memcpy(out, ai->ai_addr, sizeof(struct sockaddr));
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(sock < 0) {
        perror("socket()");
        exit(1);
    }
    struct timespec timeout = {
        .tv_sec = 0,
        // XXX SO_RCVTIMEO actually treats this as `tv_usec`
        .tv_nsec = 10000,
    };
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    return sock;
}

void net_send_keycode(int remote, uint8_t keycode, struct sockaddr *addr)
{
    if(sendto(remote, &keycode, 1, 0, addr, sizeof(struct sockaddr)) < 0)
        perror("sendto()");
}

uint8_t net_encode_scancode(uint8_t scancode, bool pressed)
{
    uint8_t pressed_u8 = pressed << 7;
    assert(pressed_u8 == 0x80 || pressed_u8 == 0);
    return (uint8_t)SDL_GetKeyFromScancode(scancode) | pressed_u8;
}

const char *net_ffmpeg_format_url(const char *proto, const char *ip_string, const char *port)
{
    size_t port_sz = strlen(port);
    size_t ip_sz = strlen(ip_string);
    size_t ffmpeg_proto_sz = strlen(proto);
    size_t total = ffmpeg_proto_sz + ip_sz + port_sz + 5;
    char *buf = calloc(total, 1);
    snprintf(buf,
             total,
             "%s://%s:%s",
             proto,
             ip_string,
             port);
    printf("URL %s\n", buf);

    return buf;
}
