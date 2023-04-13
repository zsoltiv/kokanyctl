#include <ifaddrs.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

#include "SDL_net.h"

#include "utils.h"

#include "net.h"

#define NET_INTERFACE "wlan0"
#define NET_REMOTE "192.168.0.1"

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
