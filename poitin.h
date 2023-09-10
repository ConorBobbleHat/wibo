#include "common.h"
#include <sys/mman.h>
#include <arpa/inet.h>

#ifndef POITIN_H
#define POITIN_H

namespace poitin {
    static int socketFd;
    static sockaddr_in server;

    enum Register {
        EAX,
        EBX,
        ECX,
        EDX,
        ESI,
        EDI,
        EIP,
        ESP,
        EBP
    };

    enum Executor {
        WINDOWS,
        WIBO
    };

    bool init(TIB *tib);
    void runWindowsSyscall();
    uint32_t fetchRegister(Register r);
    bool checkAddressMapped(void* addr, Executor executor);
    void* substituteDynamicPointer(void* addr);

    void memcpy(void* addr, size_t len);
    size_t strlen(char* addr);
    size_t strlenWide(uint16_t* addr);

    bool malloc(void* addr, size_t len);

    void ret();
}

#endif