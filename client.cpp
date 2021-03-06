//
// Created by roman on 20.05.19.
//

#include <memory.h>
#include <arpa/inet.h>
#include "client.h"
#include "my_error.h"
#include <unistd.h>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>

using std::runtime_error;
using std::string;
using std::cin;
using std::cout;

client::client() : tcp_socket(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) {
    if (tcp_socket == -1) {
        my_error("Socket cannot be created");
        throw runtime_error("Socket cannot be created");
    }
}

void client::start(const char *hostAddress, const in_port_t port) {
    struct sockaddr_in server{};
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    server.sin_port = port;

    if (inet_aton(hostAddress, &server.sin_addr) == 0) {
        my_error("Internet host address is invalid");
        throw runtime_error("Internet host address is invalid");
    }
    my_fd epoll_fd(epoll_create(1));
    if (epoll_fd == -1) {
        my_error("Cannot create epoll instance");
        throw runtime_error("Cannot create epoll instance");
    }
    bool connected = false;
    if (connect(tcp_socket, reinterpret_cast<sockaddr *>(&server), sizeof(sockaddr_in)) == -1) {
        if (errno == EINPROGRESS) {
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = tcp_socket;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket, &ev) == -1) {
                my_error("Cannot invoke epoll_ctl on server");
                throw runtime_error("Cannot invoke epoll_ctl on server");
            }
        } else {
            my_error("Cannot connect");
            throw runtime_error("Cannot connect");
        }
    } else {
        connected = true;
        ev.events = EPOLLIN;
        ev.data.fd = tcp_socket;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket, &ev) == -1) {
            my_error("Cannot invoke epoll_ctl on server");
            throw runtime_error("Cannot invoke epoll_ctl on server");
        }
    }

    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        my_error("Cannot add stdin");
        throw runtime_error("Cannot add stdin");
    }

    bool flag = true;
    cout << "Type your requests" << std::endl;
    while (flag) {
        int ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (ready == -1) {
            my_error("epoll_wait problem");
        }
        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == STDIN_FILENO && connected) {
                string message;
                getline(cin, message);
                if (message == "STOP" || !cin) {
                    flag = false;
                    continue;
                }
                do_request(message);
            } else if (events[i].data.fd == tcp_socket) {
                if ((events[i].events & EPOLLERR) || (connected && (events[i].events & EPOLLHUP))) {
                    my_error("Problem with connection");
                    flag = false;
                    break;
                }
                if (events[i].events & EPOLLIN) {
                    ssize_t readed = receive_response(); //read read read...but...
                    if (readed < 0) {
                        my_error("Error while receiving data from server");
                    }
                    if (readed == 0) {
                        flag = false;
                        continue;
                    }
                    cout << "[RESPONSE] " << std::string(buffer, readed) << std::endl;
                }
                if (events[i].events & EPOLLOUT) {
                    if (!connected) {
                        int error = 0;
                        socklen_t length = sizeof(error);
                        if (getsockopt(tcp_socket, SOL_SOCKET, SO_ERROR, &error, &length) == -1) {
                            my_error("Error with getsockopt");
                            flag = false;
                            break;
                        }
                        if (error != 0) {
                            flag = false;
                            my_error("Cannot connect");
                            break;
                        }
                        ev.events = EPOLLIN;
                        ev.data.fd = events[i].data.fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &ev) == -1) {
                            my_error("Cannot change mod");
                            flag = false;
                            break;
                        }
                        connected = true;
                    }
                }
            }
        }
    }
}

ssize_t client::receive_response() {
    ssize_t count = 0;
    {
        ssize_t received = 0;
        while (received != sizeof(ssize_t)) {
            ssize_t tmp = read(tcp_socket, (char *) (&count) + received, sizeof(ssize_t) - received);
            if (tmp == -1 || tmp == 0) {
//                my_error("Cannot response");
                return tmp;
            }
            received += tmp;
        }
    }
    {
        ssize_t received = 0;
        while (received != count) {
            ssize_t tmp = read(tcp_socket, buffer + received, count - received);
            if (tmp == -1 || tmp == 0) {
//                my_error("Cannot response");
                return tmp;
            }
            received += tmp;
        }
    }
    return count;
}

void client::do_request(const string &msg) {
    ssize_t count = msg.length() + 1;
    {
        ssize_t sent = 0;
        while (sent != sizeof(ssize_t)) {
            ssize_t written = write(tcp_socket, (char *) (&count) + sent, sizeof(ssize_t) - sent);
            if (written == -1) {
                my_error("Cannot do request");
                return;
            }
            sent += written;
        }
    }
    {
        ssize_t sent = 0;
        while (sent != count) {
            ssize_t written = write(tcp_socket, msg.data() + sent, count - sent);
            if (written == -1) {
                my_error("Cannot response");
                return;
            }
            sent += written;
        }
    }

//    if (send(socket_fd, .data(), message.length() + 1, 0) != message.length() + 1) { //+1 for null terminal
//        my_error("Cannot send message");
//    }
}