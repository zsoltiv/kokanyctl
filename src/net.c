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

#define NET_PORT 1337
#define NET_FFMPEG_PROTO "tcp://"
#define NET_FFMPEG_OPTS "?listen=1"
#define NET_REMOTE "192.168.0.99"

TCPsocket net_connect_to_remote(void)
{
    IPaddress remote;
    if(SDLNet_ResolveHost(&remote, NET_REMOTE, htons(NET_PORT)) < 0)
        ctl_die("SDLNet_ResolveHost(): %s\n", SDLNet_GetError());
    printf("%u:%u\n", remote.host, remote.port);
    fflush(stdout);
    TCPsocket sock =  SDLNet_TCP_Open(&remote);
    if(!sock)
        ctl_die("SDLNet_TCP_Open(): %s\n", SDLNet_GetError());

    return sock;
}

void net_send_keycode(TCPsocket remote, uint8_t keycode)
{
    printf("sending %u\n", keycode);
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
    snprintf(port, sizeof(port), ":%u", NET_PORT);

    size_t port_sz = strlen(port);
    size_t ip_sz = strlen(ip_string);
    size_t ffmpeg_opts_sz = strlen(NET_FFMPEG_OPTS);
    size_t ffmpeg_proto_sz = strlen(NET_FFMPEG_PROTO);
    size_t total = ffmpeg_proto_sz + ip_sz + port_sz + ffmpeg_opts_sz + 1;
    char *buf = calloc(total + 1, 1);
    snprintf(buf,
             total,
             "%s%s%s%s",
             NET_FFMPEG_PROTO,
             ip_string,
             port,
             NET_FFMPEG_OPTS);

    return buf;
}
