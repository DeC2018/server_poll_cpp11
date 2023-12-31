#include <iostream>
#include <algorithm>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define POLL_SIZE 2048

int iostl(int fd, int fiobio, int *pInt);
int set_nonblock(int fd) {
    int flags;
#if defined(NONBLOCK)
    if(-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | 0_NONBLOCK);
#else
    flags = 1;
    int FIOBIO;
    return iostl(fd, FIOBIO, &flags);
#endif
}
int iostl(int fd, int fiobio, int *pInt) {
    return 0;
}

int main(int argc_, char **argv) {
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    std::set<int> SlaveSockets;

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(MasterSocket, (struct sockaddr *) (&SockAddr), sizeof(SockAddr));

    set_nonblock(MasterSocket);

    listen(MasterSocket, SOMAXCONN);

    struct pollfd Set[POLL_SIZE];
    Set[0].fd = MasterSocket;
    Set[0].events = POLLIN;

    while (true) {
        unsigned int Index = 1;
        for (auto Iter = SlaveSockets.begin();
             Iter != SlaveSockets.end();
             Iter++) {
            Set[Index].fd = *Iter;
            Set[Index].events = POLLIN;
            Index++;
        }
        unsigned int SetSize = 1 + SlaveSockets.size();

        poll(Set, SetSize, -1);

        for (unsigned int i = 0; i < SetSize; i++) {
            if (Set[i].revents & POLLIN) {
                if (i) {
                    static char Buffer[1024];
                    int RecvSize = recv(Set[i].fd,
                                        Buffer,
                                        1024,
                                        MSG_NOSIGNAL);
                    if ((RecvSize == 0) && (errno != EAGAIN)) {
                        shutdown(Set[i].fd, SHUT_RDWR);
                        close(Set[i].fd);
                        SlaveSockets.erase(Set[i].fd);
                    } else if (RecvSize > 0) {
                        send(Set[i].fd,Buffer,RecvSize,MSG_NOSIGNAL);
                    }
                } else {
                    int SlaveSocket =
                            accept(MasterSocket, 0, 0);
                    set_nonblock(SlaveSocket);
                    SlaveSockets.insert(SlaveSocket);
                }
            }
        }
    }
    return 0;
}
