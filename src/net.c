#include <ifaddrs.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

#include "SDL_net.h"

#include "utils.h"

#include "net.h"

#define NET_INTERFACE "wlan0"
#define NET_PORT 1337u
#define NET_FFMPEG_PROTO "tcp://"
#define NET_FFMPEG_OPTS "?listen=1"
#define NET_REMOTE "127.0.0.1"

void net_get_local_address(IPaddress *ip)
{
    struct ifaddrs *interfaces, *ifap;
    getifaddrs(&interfaces);
    ifap = interfaces;
    do {
        if(!strcmp(NET_INTERFACE, ifap->ifa_name) &&
           ifap->ifa_addr->sa_family == AF_INET)
            break;
    } while((ifap = ifap->ifa_next));
    if(!ifap)
        ctl_die("net_get_local_address(): no interface named %s\n", NET_INTERFACE);
    struct sockaddr_in *inaddr = (struct sockaddr_in *)ifap->ifa_addr;
    ip->host = inaddr->sin_addr.s_addr;
    freeifaddrs(interfaces);
}

void net_resolve_kokanybot(IPaddress *ip)
{
    SDLNet_ResolveHost(ip, NET_REMOTE, 0);
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
