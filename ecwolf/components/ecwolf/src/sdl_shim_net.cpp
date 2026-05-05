#include "sdl_shim_net.h"
#include <stdlib.h>

extern "C" {
int SDLNet_Init(void) { return 0; }
void SDLNet_Quit(void) {}
const char *SDLNet_GetError(void) { return "Net not supported"; }

UDPsocket SDLNet_UDP_Open(Uint16 port) { return NULL; }
void SDLNet_UDP_Close(UDPsocket sock) {}
UDPpacket *SDLNet_AllocPacket(int size) {
    UDPpacket *p = (UDPpacket *)malloc(sizeof(UDPpacket));
    p->data = (Uint8 *)malloc(size);
    p->maxlen = size;
    return p;
}
void SDLNet_FreePacket(UDPpacket *packet) {
    if (packet) {
        free(packet->data);
        free(packet);
    }
}
int SDLNet_UDP_Send(UDPsocket sock, int channel, UDPpacket *packet) { return 0; }
int SDLNet_UDP_Recv(UDPsocket sock, UDPpacket *packet) { return 0; }
int SDLNet_ResolveHost(IPaddress *address, const char *host, Uint16 port) { return -1; }

}
