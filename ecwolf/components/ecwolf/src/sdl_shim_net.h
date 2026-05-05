#ifndef SDL_NET_SHIM_H
#define SDL_NET_SHIM_H

#include "sdl_shim.h"

typedef struct {
    Uint32 host;
    Uint16 port;
} IPaddress;

typedef struct {
    int channel;
    Uint8 *data;
    int len;
    int maxlen;
    int status;
    IPaddress address;
} UDPpacket;

typedef struct _UDPsocket *UDPsocket;

#ifdef __cplusplus
extern "C" {
#endif

int SDLNet_Init(void);
void SDLNet_Quit(void);
const char *SDLNet_GetError(void);

UDPsocket SDLNet_UDP_Open(Uint16 port);
void SDLNet_UDP_Close(UDPsocket sock);
UDPpacket *SDLNet_AllocPacket(int size);
void SDLNet_FreePacket(UDPpacket *packet);
int SDLNet_UDP_Send(UDPsocket sock, int channel, UDPpacket *packet);
int SDLNet_UDP_Recv(UDPsocket sock, UDPpacket *packet);
int SDLNet_ResolveHost(IPaddress *address, const char *host, Uint16 port);

#ifdef __cplusplus
}
#endif

#endif
