#include "poitin.h"

namespace poitin {
    bool init(TIB *tib) {
        // Step one: connect to the poitin socket.
        // This allows to to request & receive information about the execution of
        // our windows counterpart.
        socketFd = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketFd < 0) {
            perror("Error creating socket");
            return false;
        }

        memset(&server, 0, sizeof(server));

        server.sin_family    = AF_INET; // IPv4
        server.sin_addr.s_addr = inet_addr("172.25.96.1");
        server.sin_port = htons(8088);

        if (connect(socketFd, (const struct sockaddr*) &server, sizeof(server)) < 0) {
            perror("Error binding socket");
            return false;
        }

        // Step two: setup our stack to be in the same location as our windows counterpart
        if (!getenv("POITIN_STACK_BASE")) {
            fprintf(stderr, "Expected POITIN_STACK_BASE to be set!\n");
            return false;
        }	
        
        void* stackBase = (void*) atoi(getenv("POITIN_STACK_BASE"));
        void* stackBasePageAligned = (void*)(((uint32_t)stackBase | (getpagesize() - 1)) + 1);

        // Make sure to map our stack with the correct permissions
        // before we attempt to use it
        int stackSize = 0x1000;
        
        void* stack = mmap(stackBasePageAligned - stackSize, stackSize, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
        if (stack == MAP_FAILED) {
            perror("Image mapping failed");
            return false;
        }
        
        memset(stackBasePageAligned - stackSize, 0, stackSize);

        tib->sehFrame = stackBase + 0x48; // hacky :(. TODO: rework with SEH implementation
        tib->stackLimit = stackBasePageAligned;

        return true;
    }

    void runWindowsSyscall() {
        // length 2, opcode 0
        char buf[] = { 2, 0 };
        sendto(socketFd, &buf, buf[0], 0, (const struct sockaddr*) &server, sizeof(server));
    
        asm("int3");
    }

    uint32_t fetchRegister(Register r) {
        // length 3, opcode 1
        char buf[] = { 3, 1, r };
        sendto(socketFd, &buf, buf[0], 0, (const struct sockaddr*) &server, sizeof(server));

        asm("int3");

        uint32_t val = 0;
        recvfrom(socketFd, (char*) &val, 4, 0, NULL, NULL);
        return val;
    }

    struct CheckAddressRequest {
        char len;
        char opcode;
        uint32_t address;
        uint32_t executor;
    };

    bool checkAddressMapped(void* addr, Executor executor) {
        CheckAddressRequest req;
        req.len = sizeof(CheckAddressRequest);
        req.opcode = 6;
        req.address = (uint32_t)addr;
        req.executor = (uint32_t)executor;

        sendto(socketFd, &req, req.len, 0, (const struct sockaddr*) &server, sizeof(server));

        asm("int3");

        uint32_t val = 0;
        recvfrom(socketFd, (char*) &val, 4, 0, NULL, NULL);
        return val != 0;
    }

    struct SubstituteDynamicPointerRequest {
        char len;
        char opcode;
        uint32_t address;
    };

    void* substituteDynamicPointer(void* addr) {
        SubstituteDynamicPointerRequest req;
        req.len = sizeof(SubstituteDynamicPointerRequest);
        req.opcode = 7;
        req.address = (uint32_t)addr;

        sendto(socketFd, &req, req.len, 0, (const struct sockaddr*) &server, sizeof(server));

        asm("int3");

        void* val = 0;
        recvfrom(socketFd, (char*) &val, 4, 0, NULL, NULL);
        return val;
    }

    struct MemcpyRequest {
        char len;
        char opcode;
        uint32_t address;
        uint32_t copyLen;
    };

    void memcpy(void* addr, size_t len) {
        MemcpyRequest req;
        req.len = sizeof(MemcpyRequest);
        req.opcode = 3;
        req.address = (uint32_t)addr;
        req.copyLen = len;

        sendto(socketFd, &req, req.len, 0, (const struct sockaddr*) &server, sizeof(server));

        asm("int3");

        recvfrom(socketFd, addr, len, 0, NULL, NULL);
    }

    struct StrlenRequest {
        char len;
        char opcode;
        uint32_t address;
    };

    size_t strlen(char* addr) {
        StrlenRequest req;
        req.len = sizeof(StrlenRequest);
        req.opcode = 4;
        req.address = (uint32_t)addr;

        sendto(socketFd, &req, req.len, 0, (const struct sockaddr*) &server, sizeof(server));

        asm("int3");

        uint32_t val = 0;
        recvfrom(socketFd, (char*) &val, 4, 0, NULL, NULL);
        return val;
    }

    size_t strlenWide(uint16_t* addr) {
        StrlenRequest req;
        req.len = sizeof(StrlenRequest);
        req.opcode = 5;
        req.address = (uint32_t)addr;

        sendto(socketFd, &req, req.len, 0, (const struct sockaddr*) &server, sizeof(server));

        asm("int3");

        uint32_t val = 0;
        recvfrom(socketFd, (char*) &val, 4, 0, NULL, NULL);
        return val;
    }

    void ret() {
        // length 2, opcode 2
        char buf[] = { 2, 2 };
        sendto(socketFd, &buf, buf[0], 0, (const struct sockaddr*) &server, sizeof(server));

        asm("int3");
    }

    bool malloc(void* addr, size_t len) {
        void* alignedAddress = (void*) ((uint32_t)addr & ~(getpagesize() - 1));
        size_t alignedLen = len + ((uint32_t)addr - (uint32_t)alignedAddress);

        void* stack = mmap(alignedAddress, alignedLen, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
        if (stack == MAP_FAILED) {
            perror("Image mapping failed");
            return false;
        }

        return true;
    }
}