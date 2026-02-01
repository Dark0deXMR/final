#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define RATE "300000"
#define OUTFILE "bios.txt"

#define GREEN  "\033[92m"
#define WHITE  "\033[97m"
#define RESET  "\033[0m"

void usage(char *p) {
    printf("Uso: %s <rango> -p<puerto>\n", p);
    printf("Ejemplo:\n");
    printf("  %s 114 -p22\n", p);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strncmp(argv[2], "-p", 2) != 0)
        usage(argv[0]);

    char *range = argv[1];
    char *port  = argv[2] + 2;

    FILE *out = fopen(OUTFILE, "w");
    if (!out) {
        perror("bios.txt");
        return 1;
    }

    int pipefd[2];
    pipe(pipefd);

    pid_t banner_pid = fork();
    if (banner_pid == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        execl("./banner", "banner", "-", port, "2000", NULL);
        perror("banner");
        exit(1);
    }
    close(pipefd[0]);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "masscan %s.0.0.0/8 -p%s --rate %s --wait 0",
        range, port, RATE
    );

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("masscan");
        return 1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char ip[64];

        if (sscanf(line, "%*[^o]on %63s", ip) == 1) {
            fprintf(out, "%s\n", ip);
            fflush(out);

            dprintf(pipefd[1], "%s\n", ip);

            printf(GREEN "[SCAN] " WHITE "%s\n" RESET, ip);
        }
    }

    pclose(fp);
    fclose(out);
    close(pipefd[1]);

    wait(NULL);

    printf(GREEN "[✓] Scan finalizado\n" RESET);
    return 0;
}
