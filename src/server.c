/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Reliable UDP Server
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "server_func.c"


/*
 * Global variables
 */
extern int verbose;
extern int for_test;


/*
 * Zombie Killer
 */
static void wait_for_child(int sig)
{
    unused(sig);
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


/*
 * Main
 */
int main(int argc, char const *argv[])
{
    int server_port = 0;
    int cmd_type = -1;
    int win_len = -1;
    double prob = -1;
    time_t timeout = -1;
    char fname[LEN] = "";
    long filesize = 0;
    int sockfd;
    struct sigaction sa;

    check_arguments(argc, argv, &server_port);
    father_udp_set(&sockfd, server_port);

    sa.sa_handler = wait_for_child;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    abort_on_error(system("clear") == -1, "system() error in server");
    printf(GREEN "SERVER READY...\n" DEFAULT);

    forever {

        pid_t pid;
        int child_socket;

        pid = fork();

        if (!pid) {
            // child
            child_udp_set(&child_socket);
            child_job(child_socket, cmd_type, fname, filesize, win_len, prob, timeout);
            abort_on_error(close(child_socket) == -1, "close() error in server");
            exit(EXIT_SUCCESS);
        } else {
            if (pid == -1) {
                perror("fork() error");
                exit(EXIT_FAILURE);
            } else {
                // father
                father_job(sockfd, &cmd_type, fname, &filesize, &win_len, &prob, &timeout);
            }
        }
    }
}
