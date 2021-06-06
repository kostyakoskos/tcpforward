#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>

int main(int argc, char const *argv[])
{
    int connected = 0;
    int number_of_connections = atoi(argv[1]);
    int port[2];
    port[0] = atoi(argv[2]);
    port[1] = atoi(argv[3]);
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    char buf[128];
    struct pollfd *poll_fds;
    poll_fds = (struct pollfd*)calloc(number_of_connections * 2, sizeof(struct pollfd));
    int opt = 1;
    int addrlen = sizeof(address);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port[0]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port[1]);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    listen(server_fd, number_of_connections);

    poll_fds[0].fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    poll_fds[0].events = POLLIN;
    poll_fds[number_of_connections].fd = socket(AF_INET, SOCK_STREAM, 0);
    poll_fds[number_of_connections].events = POLLIN;
    connect(poll_fds[number_of_connections].fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    connected++;

    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);

    while(1){
        int fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if(fd > 0){
            for (int i = 0; i < number_of_connections; i++){
                if (poll_fds[i].fd == 0){
                    poll_fds[i].fd = fd;
                    poll_fds[i].events = POLLIN;
                    poll_fds[i + number_of_connections].fd = socket(AF_INET, SOCK_STREAM, 0);
                    poll_fds[i + number_of_connections].events = POLLIN;
                    connect(poll_fds[i + number_of_connections].fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
                    connected++;
                    break;
                }
            }
        }

        bzero(buf, 128);
        if(poll(poll_fds, (nfds_t)(number_of_connections * 2), 0) <= 0){
            continue;
        }

        for (int i = 0; i < number_of_connections * 2; i++){
            if (poll_fds[i].fd == 0){
                continue;
            }
            if (poll_fds[i].revents == POLLIN){
                int rc = 0;
                rc = recv(poll_fds[i].fd, buf, 1, 0);
                if (rc > 0){
                    send(poll_fds[(i + number_of_connections) % (number_of_connections * 2)].fd, buf, 1, 0);
                }
                else{
                    shutdown(poll_fds[i].fd, O_RDWR);
                    close(poll_fds[i].fd);
                    poll_fds[i].fd = 0;
                }
                do{
                    int sl = 0;
                    rc = recv(poll_fds[i].fd, buf, 128, 0);
                    do{
                        sl += send(poll_fds[(i + number_of_connections) % (number_of_connections * 2)].fd, buf, rc - sl, 0);
                    }
                    while(sl < rc);
                }
                while(rc >= 128);
                break;
            }
        }
    }

    free(port);
    return 0;
}
