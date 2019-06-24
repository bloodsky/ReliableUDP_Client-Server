/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Reliable UDP Client
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

#include "client_func.c"


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
    int sockfd;
    const char *host = NULL;
    int port = 0;
    int cmd_type = -1;
    int win_len;
    double prob;
    time_t timeout;
    char in_buff[IN_BUFF];
    char cmd[LEN] = "";
    char fname[LEN] = "";
    struct sigaction sa;

    check_arguments(argc, argv, &host, &port, &win_len, &prob, &timeout);

    set_udp_socket(&sockfd, host, port);

    abort_on_error(system("clear") == -1, "system() error in client");

    sa.sa_handler = wait_for_child;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    forever {

        pid_t pid;

        pid = fork();

        if (!pid) {
            // child
            do_command(sockfd, cmd_type, fname, win_len, prob, timeout);
            abort_on_error_client_child(close(sockfd) == -1, "close() error in client");
            exit(EXIT_SUCCESS);
        } else {
            if (pid == -1) {
                perror("fork() error");
                exit(EXIT_FAILURE);
            } else {
                // father
                cmd_type = -1;
                // every line parse input and take the command
                if (fgets(in_buff, IN_BUFF, stdin) != NULL)
                    parse_input(in_buff, cmd, &cmd_type, fname);
            }
        }
    }
}
