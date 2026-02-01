#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/select.h>

#define WHITE "\033[97m"
#define RED   "\033[91m"
#define RESET "\033[0m"

int connect_timeout(int sock, struct sockaddr *addr, socklen_t len, int timeout_ms) {
    fcntl(sock, F_SETFL, O_NONBLOCK);
    connect(sock, addr, len);

    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);

    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    if (select(sock + 1, NULL, &fdset, NULL, &tv) == 1) {
        int so_error;
        socklen_t slen = sizeof so_error;
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &slen);
        return so_error;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <ips.txt | -> <puerto> <timeout_ms>\n", argv[0]);
        return 1;
    }

    FILE *in = (strcmp(argv[1], "-") == 0) ? stdin : fopen(argv[1], "r");
    if (!in) {
        perror("input");
        return 1;
    }

    int port    = atoi(argv[2]);
    int timeout = atoi(argv[3]);

    FILE *blog = fopen("banner.log", "w");
    FILE *ips  = fopen("ips.lst", "w");

    if (!blog || !ips) {
        perror("output");
        return 1;
    }

    char ip[64];
    while (fgets(ip, sizeof(ip), in)) {
        ip[strcspn(ip, "\n")] = 0;

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;

        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);

        if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
            close(sock);
            continue;
        }

        if (connect_timeout(sock, (struct sockaddr *)&addr, sizeof(addr), timeout) != 0) {
            close(sock);
            continue;
        }

        char banner[256] = {0};
        recv(sock, banner, sizeof(banner) - 1, 0);
        close(sock);

        if (!banner[0]) continue;

        banner[strcspn(banner, "\r\n")] = 0;

        printf(WHITE "%s -- " RED "%s" RESET "\n", ip, banner);
        fflush(stdout);

        fprintf(blog, "%s -- %s\n", ip, banner);
        fflush(blog);

        if (strstr(banner, "OpenSSH")) {
            fprintf(ips, "%s\n", ip);
            fflush(ips);
        }
    }

    fclose(blog);
    fclose(ips);
    return 0;
}
