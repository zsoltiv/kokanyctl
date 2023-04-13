#ifndef NET_H
#define NET_H

#include "SDL_net.h"

void net_get_local_address(IPaddress *ip);
void net_resolve_kokanybot(IPaddress *ip);

#endif /* NET_H */
