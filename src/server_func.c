/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Reliable UDP Server Functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "constants.h"
#include "macro.h"
#include"file_io.c"
#include "packet.h"
#include "send.c"
#include "receive.c"
#ifndef COMM_FUNC
#define COMM_FUNC "common_func.c"
#include COMM_FUNC
#endif


/*
 * Prototypes
 */
void check_arguments(int, const char **, int *);
int usage(FILE *, const char *);
void father_udp_set(int *, int);
void father_job(int, int *, char *, long *, int *, double *, long *);
void child_udp_set(int *);
void child_job(int, int, char *, long, int, double, long);
void serv_list(int, int, double, long);
void serv_get(int, char *, int, double, long);
void serv_put(int, char *, long, int, double, long);


/*
 * Global variables
 */
int verbose, for_test;
struct sockaddr_in client_address, child_address;
socklen_t client_len = sizeof(client_address);
socklen_t child_len = sizeof(child_address);


/*
 * Functions
 */
void check_arguments(int argc, const char *argv[], int *server_port)
{
    if (argc < 2 || argc > 3) {
        input_error(stdout, argv[0]);

    } else if (argc == 2) {

        if (argv[1] != NULL && !strcmp(argv[1], "-h"))
            usage(stdout, argv[0]);
        else if (strtol(argv[1], NULL, 10) < 49152 || strtol(argv[1], NULL, 10) > 65535) {
            printf("Invalid port number\n");
            input_error(stdout, argv[0]);
        } else if (strtol(argv[1], NULL, 10) >= 49152 && strtol(argv[1], NULL, 10) <= 65535) {
            *server_port = strtol(argv[1], NULL, 10);
            vprintf("Server Port: %d\n", *server_port);
        } else
            input_error(stdout, argv[0]);

    } else if (argc == 3) {

        if (strtol(argv[1], NULL, 10) < 49152 || strtol(argv[1], NULL, 10) > 65535) {
            printf("Invalid port number\n");
            input_error(stdout, argv[0]);
        } else if (strtol(argv[1], NULL, 10) >= 49152 && strtol(argv[1], NULL, 10) <= 65535) {
            *server_port = strtol(argv[1], NULL, 10);
            vprintf("Server Port: %d\n", *server_port);
        } else
            input_error(stdout, argv[0]);

        if (argv[2] != NULL && !strcmp(argv[2], "-v")) {
            verbose = 1;
            // vprintf("Verbose: %d\n", verbose);
        } else if (argv[2] != NULL && !strcmp(argv[2], "-t")) {
            for_test = 1;
        } else if (argv[2] != NULL && !strcmp(argv[2], "-vt")) {
            verbose = 1;
            for_test = 1;
        } else {
            printf("Did you mean verbose option -v? For-test option -t? Or both -vt?\n");
            input_error(stdout, argv[0]);
        }
    }
}

int usage(FILE * stream, const char *program_name)
{
    fprintf(stream, BOLD_ON UNDERLINED_ON "\nRUDP Server\n" DEFAULT);
    fprintf(stream, BOLD_ON "USAGE:" DEFAULT " %s <port> [-v]\n", program_name);
    fprintf(stream, BOLD_ON "USAGE:" DEFAULT " %s -h\n", program_name);
    fprintf(stream, BOLD_ON "OPTIONS:\n" DEFAULT);
    fprintf(stream, "   -h, print this help\n");
    fprintf(stream, "   -v, print more info\n");
    fprintf(stream, "   -t, enable logging on files\n");
    fprintf(stream, "   -vt, print more info and enable logging on files\n");
    fprintf(stream, BOLD_ON "PORT CAN BE:\n" DEFAULT);
    fprintf(stream, "   a number between 49152 and 65535\n\n");
    exit(EXIT_SUCCESS);
}

void father_udp_set(int *father_sock, int port)
{
    abort_on_error((*father_sock =
                    socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0, "socket() father error");
    memset((void *) &client_address, 0, client_len);
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(port);

    abort_on_error(bind(*father_sock, (struct sockaddr *) &client_address,
                        client_len) < 0, "bind() error");
}

void father_job(int sockfd, int *cmd_type, char *fname, long *filesize,
                int *win_len, double *prob, long *timeout)
{
    char buf[MAX_PK_DATA_SIZE];

    // receive command
    recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &client_address, &client_len, -1, 1,
                 0.0, 1000000, "", 0, 0, 0);
    vprintf("command buffer received from %s: [%s]\n", inet_ntoa(client_address.sin_addr), buf);

    // parse command and setting variables
    char *tok[6];

    tok[0] = strtok(buf, SEP);
    *cmd_type = strtol(tok[0], NULL, 0);
    vprintf("cmd_type: %d\n", *cmd_type);

    tok[1] = strtok(NULL, SEP);
    strcpy(fname, tok[1]);
    vprintf("fname: %s\n", fname);

    if (*cmd_type == 2) {       // PUT
        tok[2] = strtok(NULL, SEP);
        *filesize = strtol(tok[2], NULL, 0);
        vprintf("filesize: %ld\n", *filesize);
    }

    tok[3] = strtok(NULL, SEP);
    *win_len = strtol(tok[3], NULL, 0);
    vprintf("win_len: %d\n", *win_len);

    tok[4] = strtok(NULL, SEP);
    *prob = strtod(tok[4], NULL);
    vprintf("prob: %.1f\n", *prob);

    tok[5] = strtok(NULL, SEP);
    *timeout = strtol(tok[5], NULL, 0);
    vprintf("timeout: %ld\n", *timeout);
}

void child_udp_set(int *child_sock)
{
    abort_on_error((*child_sock =
                    socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0, "socket() child error");
    memset((void *) &child_address, 0, child_len);
    child_address.sin_family = AF_INET;
    child_address.sin_addr.s_addr = client_address.sin_addr.s_addr;
    child_address.sin_port = client_address.sin_port;
}

void child_job(int sockfd, int cmd_type, char *fname, long filesize, int win_len, double prob,
               long timeout)
{
    switch (cmd_type) {
    case 0:                    // LIST
        serv_list(sockfd, win_len, prob, timeout);
        break;
    case 1:                    // GET
        serv_get(sockfd, fname, win_len, prob, timeout);
        break;
    case 2:                    // PUT
        serv_put(sockfd, fname, filesize, win_len, prob, timeout);
        break;
    };
}

void serv_list(int sockfd, int win_len, double prob, long timeout)
{
    FILE *file;
    long filesize;
    char buf[MAX_PK_DATA_SIZE];
    // struct timeval begin;
    // struct timeval end;
    // double time_spent;

    file = fopen(LSFILES, "w+");
    abort_on_error(file == NULL, "fopen() in serv_list() error");
    write_list(SFILES, file);
    abort_on_error(fseek(file, 0L, SEEK_END) == -1, "fseek() in serv_list() error");
    filesize = ftell(file);
    vprintf("filesize: %ld bytes\n", filesize);
    abort_on_error(fseek(file, 0L, SEEK_SET) == -1, "fseek() in serv_list() error");
    fclose(file);

    srand((unsigned int) time(NULL) + 1500);

    // send filesize of List "LSFILES"
    sprintf(buf, "%ld", filesize);
    sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
               cmd_type, win_len, prob, timeout, "", 0, 0, 0);

    // abort_on_error(gettimeofday(&begin, NULL) == -1, "gettimeofday() in server_func error");

    // send List "LSFILES"
    sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
               cmd_type, win_len, prob, timeout, LSFILES, filesize, 0, 0);

    printf("filelist sent\n");
    // abort_on_error(gettimeofday(&end, NULL) == -1, "gettimeofday() in server_func error");
    // time_spent = (end.tv_sec*1000000+end.tv_usec) - (begin.tv_sec*1000000+begin.tv_usec);
    // print_time(time_spent, "", NULL);
}

void serv_get(int sockfd, char *fname, int win_len, double prob, long timeout)
{
    char buf[MAX_PK_DATA_SIZE];

    srand((unsigned int) time(NULL) + 2500);

    // create path
    char *fpath;
    fpath = createpath(fname, 1);

    //check file
    //if it not exist send error message
    file = fopen(fpath, "r");
    // abort_on_error(fd == -1, "open() in serv_list() error");
    if (file == NULL) {
        printf("file '%s' doesn't exist\n", fname);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, FNE_FLAG, strlen(FNE_FLAG));
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
                   cmd_type, win_len, prob, timeout, "", 0, 0, 0);
    }
    //if it exist send OK_FLAG and send file
    else {
        long filesize = 0;
        struct timeval begin;
        struct timeval end;
        double time_spent;
        char buf_md5[LINE_LENGTH];

        printf("file '%s' exist, sending...\n", fname);

        abort_on_error(file == NULL, "fopen() in serv_get() error");
        abort_on_error(fseek(file, 0L, SEEK_END) == -1, "fseek() in serv_get() error");
        filesize = ftell(file);
        vprintf("filesize: %ld bytes\n", filesize);
        abort_on_error(fseek(file, 0L, SEEK_SET) == -1, "fseek() in serv_get() error");
        fclose(file);

        memset(buf, 0, sizeof(buf));
        memcpy(buf, OK_FLAG, strlen(OK_FLAG));
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
                   cmd_type, win_len, prob, timeout, "", 0, 0, 0);

        // send filesize of file
        sprintf(buf, "%ld", filesize);
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
                   cmd_type, win_len, prob, timeout, "", 0, 0, 0);

        abort_on_error(gettimeofday(&begin, NULL) == -1, "gettimeofday() in server_func error");

        // send file
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
                   cmd_type, win_len, prob, timeout, fpath, filesize, 0, 0);

        printf("'%s' sent\n", fname);
        abort_on_error(gettimeofday(&end, NULL) == -1, "gettimeofday() in server_func error");
        time_spent =
            (end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec);
        print_time(time_spent, "", NULL);

        memcpy(buf_md5, md5sum(buf_md5, fpath), sizeof(buf_md5));
        sprintf(buf, "%s", buf_md5);
        sendto_rel(sockfd, buf, strlen(buf),
                   (struct sockaddr *) &child_address, child_len,
                   cmd_type, winlen, 0.0, 100, "", 0, 0, 0);

    }
    free(fpath);
}

void serv_put(int sockfd, char *fname, long filesize, int win_len, double prob, long timeout)
{
    char buf[MAX_PK_DATA_SIZE];
    char buf_md5[LINE_LENGTH];
    struct timeval begin;
    struct timeval end;
    double time_spent;

    srand((unsigned int) time(NULL) + 3500);

    // create path
    char *fpath;
    fpath = createpath(fname, 1);

    //check file
    //if it not exist send error message
    file = fopen(fpath, "r");
    // abort_on_error(fd == -1, "open() in serv_list() error");
    if (file == NULL) {
        printf("file '%s' not exist, receiving...\n", fname);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, FNE_FLAG, strlen(FNE_FLAG));
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
                   cmd_type, win_len, prob, timeout, "", 0, 0, 0);
    }
    //if it exist send OK_FLAG
    else {
        printf("file '%s' exist, it will be replaced! receiving...\n", fname);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, OK_FLAG, strlen(OK_FLAG));
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address, child_len,
                   cmd_type, win_len, prob, timeout, "", 0, 0, 0);
        abort_on_error(fclose(file) == EOF, "fclose() in serv_put() error");
    }

    abort_on_error(gettimeofday(&begin, NULL) == -1, "gettimeofday() in server_func error");

    // anyway, receiving the file
    recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &child_address, &child_len,
                 cmd_type, win_len, prob, timeout, fpath, filesize, 0, 0);

    printf("'%s' received\n", fname);
    abort_on_error(gettimeofday(&end, NULL) == -1, "gettimeofday() in server_func error");
    time_spent = (end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec);
    print_time(time_spent, "", NULL);

    memcpy(buf_md5, md5sum(buf_md5, fpath), sizeof(buf_md5));
    sprintf(buf, "%s", buf_md5);
    sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &child_address,
               child_len, cmd_type, winlen, 0.0, 100, "", 0, 0, 0);

    free(fpath);
}
