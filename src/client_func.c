/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Reliable UDP Client Functions
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
#include "packet.h"
#include "file_io.c"
#include "send.c"
#include "receive.c"
#ifndef COMM_FUNC
#define COMM_FUNC "common_func.c"
#include COMM_FUNC
#endif


/*
 * Prototypes
 */
void check_arguments(int, char const **, const char **, int *, int *, double *, long *);
int usage(FILE *, char const *);
void set_udp_socket(int *, const char *, int);
void print_help(void);
void parse_input(const char *, char *, int *, char *);
void do_command(int, int, char *, int, double, long);
int send_command(int, int, char *, long *, int, double, long);
int file_on_server(int);
void do_list(int, int, int, double, long);
void do_get(int, int, char *, int, double, long);
void do_put(int, int, char *, int, double, long);
void do_print(void);


/*
 * Global variables
 */
int verbose, for_test;
struct sockaddr_in servaddr;
socklen_t len = sizeof(servaddr);


/*
 * Functions
 */
void check_arguments(int argc, char const *argv[], const char **host,
                     int *port, int *win_len, double *prob, long *timeout)
{
    if ((argc != 2) && (argc < 6 || argc > 7)) {
        input_error(stdout, argv[0]);
    }

    if (argc == 2) {
        if (!strcmp(argv[1], "-h"))
            usage(stdout, argv[0]);
        else
            input_error(stdout, argv[0]);
    }

    if (!is_valid_ip_address(argv[1])) {
        printf("Invalid Ip format\n");
        input_error(stdout, argv[0]);
    } else
        *host = argv[1];

    if (strtol(argv[2], NULL, 10) < 49152 || strtol(argv[2], NULL, 10) > 65535) {
        printf("Port number must be between 49152 and 65535\n");
        input_error(stdout, argv[0]);
    } else if (strtol(argv[2], NULL, 10) >= 49152 && strtol(argv[2], NULL, 10) <= 65535) {
        *port = strtol(argv[2], NULL, 10);
    }

    if (strtol(argv[3], NULL, 10) < 1 || strtol(argv[3], NULL, 10) > 100) {
        printf("Windows Lenght must be between 1 and 50\n");
        input_error(stdout, argv[0]);
    } else
        *win_len = strtol(argv[3], NULL, 10);

    if (strtod(argv[4], NULL) < 0.0 || strtod(argv[4], NULL) > 0.9) {
        printf("Probability must be betwenn 0 and 0.9\n");
        input_error(stdout, argv[0]);
    } else
        *prob = strtod(argv[4], NULL);  // se inserisco caratteri prob = 0.0

    if (!strcmp(argv[5], "a"))
        *timeout = -1;
    else if (strtol(argv[5], NULL, 10) < 100 || strtol(argv[5], NULL, 10) > 1000000) {
        printf("Timeout must be between 100 and 1000000\n");
        input_error(stdout, argv[0]);
    } else
        *timeout = strtol(argv[5], NULL, 10);

    if (argv[6] == NULL) {
        verbose = 0;
        for_test = 0;
    } else if (argv[6] != NULL && !strcmp(argv[6], "-v")) {
        verbose = 1;
        // vprintf("Verbose: %d\n", verbose);
    } else if (argv[6] != NULL && !strcmp(argv[6], "-t")) {
        for_test = 1;
    } else if (argv[6] != NULL && !strcmp(argv[6], "-vt")) {
        verbose = 1;
        for_test = 1;
    } else {
        printf("Did you mean verbose option -v? For-test option -t? Or both -vt?\n");
        input_error(stdout, argv[0]);
    }

    vprintf("Host: %s\n", *host);
    vprintf("Server Port: %d\n", *port);
    vprintf("Win lenght: %d\n", *win_len);
    vprintf("Probability: %f\n", *prob);
    vprintf("Timeout: %ld\n", *timeout);
}

int usage(FILE * stream, char const *program_name)
{
    fprintf(stream, BOLD_ON UNDERLINED_ON "\nRUDP Client\n" DEFAULT);
    fprintf(stream, BOLD_ON "USAGE:" DEFAULT
            " %s <address> <winlen> <prob> <timeout> [-v]\n", program_name);
    fprintf(stream, BOLD_ON "USAGE:" DEFAULT " %s -h\n", program_name);
    fprintf(stream, BOLD_ON "OPTIONS:\n" DEFAULT);
    fprintf(stream, "   -h, print this help\n");
    fprintf(stream, "   -v, print more info\n");
    fprintf(stream, "   -f, enable logging on files\n");
    fprintf(stream, "   -vf, print more info and enable logging on files\n");
    fprintf(stream, BOLD_ON "PORT CAN BE:\n" DEFAULT);
    fprintf(stream, "   a number between 49152 and 65535\n");
    fprintf(stream, BOLD_ON "WINLEN CAN BE:\n" DEFAULT);
    fprintf(stream, "   a number between 1 and 100," " the lenght of the window\n");
    fprintf(stream, BOLD_ON "PROB CAN BE:\n" DEFAULT);
    fprintf(stream, "   a number between 0.0 and 0.9," " the probability of loss packets\n");
    fprintf(stream, BOLD_ON "TIMEOUT CAN BE:\n" DEFAULT);
    fprintf(stream,
            "   a number between 100 and 1000000," " the value of the timeout in microseconds\n");
    fprintf(stream, "   the character 'a', adaptive timeout\n\n");
    exit(EXIT_SUCCESS);
}

void set_udp_socket(int *sock, const char *host, int port)
{
    if ((*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        fprintf(stderr, "Errore nella socket\n");
        exit(EXIT_FAILURE);
    }

    memset((void *) &servaddr, 0, len);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    //    is_valid_ip_address(host);
    //         OR
    abort_on_error(inet_pton(AF_INET, host, &servaddr.sin_addr) != 1,
                   "inet_pton() in client_func error");
}

void print_help(void)
{
    // system("clear");
    printf("******************************************\n");
    printf("    " BOLD_ON UNDERLINED_ON RED "UDP Client Commands:\n" DEFAULT);
    printf(BOLD_ON "list" DEFAULT ", List file stored in server\n");
    printf(BOLD_ON "get <nome_file>" DEFAULT ", Get file stored in server\n");
    printf(BOLD_ON "put <nome_file>" DEFAULT ", Send file to server\n");
    printf(BOLD_ON "print" DEFAULT ", Print file list on this client\n");
    printf(BOLD_ON "help" DEFAULT ", Print here\n");
    printf(BOLD_ON "exit" DEFAULT ", Exit\n\n");
    // printf(BOLD_ON ">" DEFAULT);
}

void parse_input(const char *in_buff, char *cmd, int *cmd_type, char *fname)
{
    char tmp1[IN_BUFF];
    // char tmp2[IN_BUFF];

    strcpy(tmp1, in_buff);
    // strcpy(tmp2, in_buff);

    if (strlen(in_buff) > 6) {

        char *tmp2;

        strtok(tmp1, NL);

        tmp2 = strtok(tmp1, SPACE);
        vprintf("cmd: %s\n", tmp2);
        strcpy(cmd, tmp2);

        if (!strcmp(cmd, GET)) {
            tmp2 = strtok(NULL, DEL);
            vprintf("fname: %s\n", tmp2);
            strcpy(fname, tmp2);

            vprintf("%s command\n", cmd);
            *cmd_type = 1;
        } else if (!strcmp(cmd, PUT)) {
            tmp2 = strtok(NULL, DEL);
            vprintf("fname: %s\n", tmp2);
            strcpy(fname, tmp2);

            vprintf("%s command\n", cmd);
            *cmd_type = 2;
        } else {
            printf(RED "Invalid command\n" DEFAULT);
            print_help();
        }
    } else if (strlen(in_buff) == 5) {

        strtok(tmp1, NL);

        if (!strcmp(tmp1, LIST)) {
            cmd = tmp1;
            vprintf("%s command\n", cmd);
            *cmd_type = 0;
        } else if (!strcmp(tmp1, HELP)) {
            cmd = tmp1;
            vprintf("%s command\n", cmd);
            *cmd_type = 4;
            print_help();
        } else if (!strcmp(tmp1, EXIT)) {
            cmd = tmp1;
            vprintf("%s command\n", cmd);
            *cmd_type = 5;
            exit(EXIT_SUCCESS);
        } else {
            printf(RED "Invalid command\n" DEFAULT);
            print_help();
        }
    } else if (strlen(in_buff) == 6) {

        strtok(tmp1, NL);

        if (!strcmp(tmp1, PRINT)) {
            cmd = tmp1;
            vprintf("%s command\n", cmd);
            *cmd_type = 3;
        } else {
            printf(RED "Invalid command\n" DEFAULT);
            print_help();
        }
    } else {
        printf(RED "Invalid command\n" DEFAULT);
        print_help();
    }
}

void do_command(int sockfd, int cmd_type, char *fname, int win_len, double prob, long timeout)
{
    switch (cmd_type) {
    case 0:                    // LIST
        do_list(sockfd, cmd_type, win_len, prob, timeout);
        break;
    case 1:                    // GET
        do_get(sockfd, cmd_type, fname, win_len, prob, timeout);
        break;
    case 2:                    // PUT
        do_put(sockfd, cmd_type, fname, win_len, prob, timeout);
        break;
    case 3:
        do_print();
        break;
    };
    printf("> ");
}

int send_command(int sockfd, int cmd_type, char *fname, long *filesize, int win_len, double prob,
                 long timeout)
{
    char *fpath;
    char buf[MAX_PK_DATA_SIZE];

    switch (cmd_type) {

    case 0:                    // LIST

        sprintf(buf, "%d,%s,%d,%.1f,%ld", cmd_type, fname, win_len, prob, timeout);
        vprintf("command buffer to send: [%s]\n", buf);

        // sending command
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &servaddr, len,
                   cmd_type, win_len, prob, timeout, "", 0, 1, 0);

        // receiving filesize of List
        char *p;
        do {
            recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &servaddr, &len, cmd_type,
                         win_len, prob, timeout, "", 0, 1, 0);
            char *tok[1];
            tok[0] = strtok(buf, SEP);
            errno = 0;
            *filesize = strtol(tok[0], &p, 0);
        } while (errno != 0 || *p != '\0');
        vprintf("filesize: %ld bytes\n", *filesize);
        return 1;

        break;

    case 1:                    // GET

        sprintf(buf, "%d,%s,%d,%.1f,%ld", cmd_type, fname, win_len, prob, timeout);
        vprintf("command buffer to send: [%s]\n", buf);

        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &servaddr, len,
                   cmd_type, win_len, prob, timeout, "", 0, 1, 0);

        //check if file exist on server, get an error if not exist
        if (!file_on_server(sockfd)) {  //it not exist
            return 0;
        } else {                //it exist
            char *p;
            do {
                // receiving filesize of file
                recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &servaddr, &len,
                             cmd_type, win_len, prob, timeout, "", 0, 1, 0);
                // vprintf("buf: %s\n", buf);
                char *tok[1];
                tok[0] = strtok(buf, SEP);
                errno = 0;
                *filesize = strtol(tok[0], &p, 0);
            } while (errno != 0 || *p != '\0');
            vprintf("filesize: %ld\n", *filesize);
            return 1;
        }
        break;

    case 2:                    // PUT

        fpath = createpath(fname, 0);
        file = fopen(fpath, "r");
        abort_on_error_client_child(file == NULL, "fopen() in send_command() error");
        abort_on_error_client_child(fseek(file, 0L, SEEK_END) == -1, "fseek() error");
        *filesize = ftell(file);
        vprintf("filesize: %ld bytes\n", *filesize);
        abort_on_error_client_child(fseek(file, 0L, SEEK_SET) == -1, "fseek() error");
        fclose(file);

        sprintf(buf, "%d,%s,%ld,%d,%.1f,%ld", cmd_type, fname, *filesize, win_len, prob, timeout);
        vprintf("command buffer to send: [%s]\n", buf);

        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &servaddr, len,
                   cmd_type, win_len, prob, timeout, "", 0, 1, 0);

        //check if file exist on server, just for info, it will be replaced
        if (!file_on_server(sockfd)) {  //it not exist
            free(fpath);
            return 0;
        } else {                //it exist
            free(fpath);
            return 1;
        }

    default:
        return -1;
        break;
    };
}

int file_on_server(int sockfd)
{
    char buf[MAX_PK_DATA_SIZE];

    recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &servaddr, &len, -1, 1, 0.0, 1000000,
                 "", 0, 1, 0);
    // printf("Received from client: [%s]\n", buf);

    // if OK_FLAG file exist on server
    if (!strncmp(buf, OK_FLAG, strlen(OK_FLAG))) {
        // vprintf("%s\n", "Filename exist on server.");
        return 1;
    }
    //filename not exist on server
    // printf("%s\n", buf);
    return 0;
}

void do_list(int sockfd, int cmd_type, int win_len, double prob, long timeout)
{
    FILE *file;
    long filesize = 0;
    char buf[MAX_PK_DATA_SIZE] = "";
    // struct timeval begin;
    // struct timeval end;
    // double time_spent;

    srand((unsigned int) time(NULL) + 1000);

    send_command(sockfd, cmd_type, "null", &filesize, win_len, prob, timeout);

    // abort_on_error(gettimeofday(&begin, NULL) == -1, "gettimeofday() in client_func error");

    recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &servaddr, &len,
                 cmd_type, win_len, prob, timeout, LCRFILES, filesize, 1, 0);

    // abort_on_error(gettimeofday(&end, NULL) == -1, "gettimeofday() in client_func error");
    // time_spent = (end.tv_sec*1000000+end.tv_usec) - (begin.tv_sec*1000000+begin.tv_usec);
    // print_time(time_spent, "", NULL);

    printf("Server Files:\n");
    file = fopen(LCRFILES, "r");
    abort_on_error_client_child(file == NULL, "fopen() in do_list() error");
    print_list(file);
    fclose(file);
}

void do_get(int sockfd, int cmd_type, char *fname, int win_len, double prob, long timeout)
{
    long filesize = 0;

    srand((unsigned int) time(NULL) + 2000);

    if (!send_command(sockfd, cmd_type, fname, &filesize, win_len, prob, timeout)) {
        printf("file '%s' doesn't exist on server\n", fname);
        printf("use the list command for the list of files\n");
    } else {
        struct timeval begin;
        struct timeval end;
        double time_spent;

        printf("file '%s' exist on server\n", fname);
        printf("receiving file...\n");

        char buf[MAX_PK_DATA_SIZE];
        char buf_md5[LINE_LENGTH];

        // create path
        char *fpath;
        fpath = createpath(fname, 0);

        abort_on_error_client_child(gettimeofday(&begin, NULL) == -1,
                                    "gettimeofday() in client_func error");

        recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &servaddr, &len,
                     cmd_type, win_len, prob, timeout, fpath, filesize, 1, 1);

        printf("\n'%s' received\n", fname);
        abort_on_error_client_child(gettimeofday(&end, NULL) == -1,
                                    "gettimeofday() in client_func error");
        time_spent =
            (end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec);
        print_time(time_spent, "", NULL);

        memcpy(buf_md5, md5sum(buf_md5, fpath), sizeof(buf_md5));
        printf("C_Md5sum: %s", buf_md5);
        socklen_t len = sizeof(servaddr);
        recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &servaddr, &len, cmd_type,
                     win_len, prob, timeout, "", 0, 1, 0);
        printf("S_Md5sum: %s", buf);

        free(fpath);
    }
}

void do_put(int sockfd, int cmd_type, char *fname, int win_len, double prob, long timeout)
{
    long filesize = 0;
    char *fpath;

    srand((unsigned int) time(NULL) + 3000);

    fpath = createpath(fname, 0);
    file = fopen(fpath, "r");

    if (file == NULL) {
        printf("file '%s' doesn't exist\n", fname);
        printf("upload some files on client\n");
    } else {
        struct timeval begin;
        struct timeval end;
        double time_spent;

        abort_on_error_client_child(fclose(file) == EOF, "fclose() in do_put() error");
        printf("file '%s' exist, sending...\n", fname);

        if (!send_command(sockfd, cmd_type, fname, &filesize, win_len, prob, timeout)) {
            printf("file '%s' doesn't exist on server\n", fname);
        } else {
            printf("file '%s' exist on server, it will be overwritten\n", fname);
        }
        // anyway sending the file
        printf("sending file...\n");

        char buf[MAX_PK_DATA_SIZE] = "";
        char buf_md5[LINE_LENGTH];

        abort_on_error_client_child(gettimeofday(&begin, NULL) == -1,
                                    "gettimeofday() in client_func error");

        // send file
        sendto_rel(sockfd, buf, strlen(buf), (struct sockaddr *) &servaddr, len,
                   cmd_type, win_len, prob, timeout, fpath, filesize, 1, 1);

        printf("\n'%s' sent\n", fname);
        abort_on_error_client_child(gettimeofday(&end, NULL) == -1,
                                    "gettimeofday() in client_func error");
        time_spent =
            (end.tv_sec * 1000000 + end.tv_usec) - (begin.tv_sec * 1000000 + begin.tv_usec);
        print_time(time_spent, "", NULL);

        memcpy(buf_md5, md5sum(buf_md5, fpath), sizeof(buf_md5));
        printf("C_Md5sum: %s", buf_md5);
        socklen_t len = sizeof(servaddr);
        recvfrom_rel(sockfd, buf, sizeof(buf), (struct sockaddr *) &servaddr, &len, cmd_type,
                     win_len, prob, timeout, "", 0, 1, 0);
        printf("S_Md5sum: %s", buf);
    }
    free(fpath);
}

void do_print()
{
    FILE *list;

    list = fopen(LCFILES, "w+");
    abort_on_error_client_child(list == NULL, "fopen() in do_print() error");
    write_list(CFILES, list);
    fclose(list);

    printf("Client Files:\n");
    list = fopen(LCFILES, "r");
    abort_on_error_client_child(list == NULL, "fopen() in do_print() error");
    print_list(list);
    fclose(list);
}
