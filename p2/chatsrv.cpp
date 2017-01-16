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

#define MAX_EVENTS 20000

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

void receive_and_send(int fd, std::set <int> &receivers, std::map <int, std::string> &messages)
{
    static char Buffer[1030];
    ssize_t RecvSize = recv(fd, Buffer, 1024, MSG_NOSIGNAL);

    if (RecvSize <= 0) {
        return;
    }
    Buffer[RecvSize] = 0;
    messages[fd] += std::string(Buffer);

    while (messages[fd].find('\n') != std::string::npos) {
        int pos = std::min(1024, int(messages[fd].find('\n')));
        std::string now = messages[fd].substr(0, pos + 1);
    	printf("%s", now.c_str());
        fflush(stdout);
        messages[fd] = messages[fd].substr(pos + 1);

        for (int to: receivers) {
            send(to, now.c_str(), int(now.size()), MSG_NOSIGNAL);
        }
    }
}

int main()
{
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int optval = 1;
    setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    optval = 1;
    setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (MasterSocket == -1) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(3100);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    int Result = bind(MasterSocket, (struct sockaddr *) &SockAddr, sizeof(SockAddr));

    if (Result == -1) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }
    set_nonblock(MasterSocket);
    Result = listen(MasterSocket, SOMAXCONN);

    if (Result == -1) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }
    struct epoll_event Event;
    Event.data.fd = MasterSocket;
    Event.events = EPOLLIN | EPOLLET;
    struct epoll_event * Events;
    Events = (struct epoll_event *) calloc(MAX_EVENTS, sizeof(struct epoll_event));
    int EPoll = epoll_create1(0);
    epoll_ctl(EPoll, EPOLL_CTL_ADD, MasterSocket, &Event);

    std::set <int> SlaveSockets;
    std::map <int, std::string> Messages;
    char greeting[] = "Welcome\n";

    while (true) {
        int N = epoll_wait(EPoll, Events, MAX_EVENTS, -1);

        for (int i = 0; i < N; i++) {
            if ((Events[i].events & EPOLLERR) || (Events[i].events & EPOLLHUP) || (Events[i].events & EPOLLRDHUP)) {
                printf("connection terminated\n");
                fflush(stdout);
 
                if (SlaveSockets.count(Events[i].data.fd)) {
                    receive_and_send(Events[i].data.fd, SlaveSockets, Messages);
                }

                shutdown(Events[i].data.fd, SHUT_RDWR);
                close(Events[i].data.fd);
                SlaveSockets.erase(SlaveSockets.find(Events[i].data.fd));
                Messages.erase(Events[i].data.fd);
            } else if (Events[i].data.fd == MasterSocket) {
                int SlaveSocket;

                while ((SlaveSocket = accept(MasterSocket, 0, 0)) != -1) {
                    printf("accepted connection\n");
                    fflush(stdout);
                    set_nonblock(SlaveSocket);

                    send(SlaveSocket, greeting, sizeof(greeting) - 1, MSG_NOSIGNAL);

                    SlaveSockets.insert(SlaveSocket);

                    struct epoll_event Event1;
                    Event1.data.fd = SlaveSocket;
                    Event1.events = EPOLLIN | EPOLLET;

                    epoll_ctl(EPoll, EPOLL_CTL_ADD, SlaveSocket, &Event1);
                }
            } else {
                receive_and_send(Events[i].data.fd, SlaveSockets, Messages);
            }
        }
    }

    return 0;
}
