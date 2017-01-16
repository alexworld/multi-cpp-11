// using Linux

#include <iostream>
#include <algorithm>
#include <set>
#include <map>
#include <cassert>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <sys/epoll.h>

#include <errno.h>
#include <string.h>

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
} 

int main()
{
    int ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (ClientSocket == -1) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(5000);
    SockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int Result = connect(ClientSocket, (struct sockaddr *) &SockAddr, sizeof(SockAddr));

    if (Result == -1) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }
    set_nonblock(ClientSocket);

    char Buffer[1030];
    struct pollfd Set[2];
    Set[0].fd = STDIN_FILENO;
    Set[0].events = POLLIN;
    Set[1].fd = ClientSocket;
    Set[1].events = POLLIN;

    while (true) {
        poll(Set, 2, -1);
        
        if (Set[0].revents & EPOLLIN) {
            fgets(Buffer, 1026, stdin);
            send(ClientSocket, Buffer, strlen(Buffer), MSG_NOSIGNAL);
        }
        if (Set[1].revents & EPOLLIN) {
            int RecvSize = recv(ClientSocket, Buffer, 1025, MSG_NOSIGNAL);
            Buffer[RecvSize] = 0;
            printf("%s", Buffer);
            fflush(stdout);
        }
    }

    return 0;
}
