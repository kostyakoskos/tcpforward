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
    int number_of_connections = atoi(argv[1]);// atoi: to int or 0
    int port[2];
    port[0] = atoi(argv[2]);
    port[1] = atoi(argv[3]);
    struct sockaddr_in address;// Sockaddr_in is a basic structure for system >
    struct sockaddr_in serv_addr;
    char buf[128];
    struct pollfd *poll_fds;// pollfd is a sctructure. pollfd(filedescripor, r>
    poll_fds = (struct pollfd*)calloc(number_of_connections * 2, sizeof(struct>
    int addrlen = sizeof(address);
    int one = 1;

address.sin_family = AF_INET;// AF_INET - my socket may use only type of a>
    address.sin_addr.s_addr = INADDR_ANY;// When new process want to get new i>
    address.sin_port = htons(port[0]);// htons: from u_short to TCP/IP

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port[1]);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);// make a structure w>

    // AIF_INET it's all types off address that can be used by socket.
    // socket(int domain, int type, int protocol). Socket create endpoint for >
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // setsockport for settings flag on socket. setsockport(int s, int level, >
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
   // bind(int sockfd, const struct sockaddr * addr, socklen_t addrlen). Set a>
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    //listen(int sockfd, int backlog). Listen connection on socket. it makes s>
    listen(server_fd, number_of_connections);

    poll_fds[0].fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*>
    poll_fds[0].events = POLLIN;// pollin- there is a data for read

poll_fds[number_of_connections].fd = socket(AF_INET, SOCK_STREAM, 0);
    poll_fds[number_of_connections].events = POLLIN;
    // connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen>
    connect(poll_fds[number_of_connections].fd, (struct sockaddr*)&serv_addr, >

    // fcntl(int fd, int cmd, long arg)
    // Set flag close-on-exec to value, by bit FD_CLOEXEC from argument arg
    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);// O_>

    while(1){
        // accept(int s, struct sockaddr *addr, socklen_t *addrlen). Accept ge>
        int fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&ad>
        if(fd > 0){
            for (int i = 0; i < number_of_connections; i++){
                if (poll_fds[i].fd == 0){
                    poll_fds[i].fd = fd;
                    poll_fds[i].events = POLLIN;// POLLIN- there is a data for>
                    poll_fds[i + number_of_connections].fd = socket(AF_INET, S>
                    poll_fds[i + number_of_connections].events = POLLIN;
                    connect(poll_fds[i + number_of_connections].fd, (struct so>
                    break;
                }

}
            }
        }

        bzero(buf, 128);
        // poll- wait event in file discriptor. poll(struct pollfd *fds, nfds_>
// 0 no real event
 if(poll(poll_fds, (nfds_t)(number_of_connections * 2), -1) <= 0){
            continue;
        }

        for (int i = 0; i < number_of_connections * 2; i++){
            if (poll_fds[i].fd == 0){
                continue;
            }
            if (poll_fds[i].revents == POLLIN){
                int rc = 0;
                // recv(int s, void *buf, size_t len, int flags). recv get a m>
                rc = recv(poll_fds[i].fd, buf, 1, 0);
                if (rc > 0){
                    send(poll_fds[(i + number_of_connections) % (number_of_con>
                }

else{
                    shutdown(poll_fds[i].fd, O_RDWR);// O_RDWR - to read or wr>
                    close(poll_fds[i].fd);
                    poll_fds[i].fd = 0;
                }
                do{
                    rc = recv(poll_fds[i].fd, buf, 128, 0);
                    for(int p = 0; p < rc;)
                    {
                        p += send(poll_fds[(i + number_of_connections) % (numb>
                    }
                }
                while(rc >= 128);// if < then 128 it mean that message less th>
                break;
            }
        }
    }

    free(port);
    return 0;
}