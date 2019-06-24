/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Common Functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "constants.h"
#include "macro.h"

/*
 * Prototypes
 */
int is_valid_ip_address(char const *);
int input_error(FILE *, char const *);
void print_time(long, const char *, FILE *);
char *md5sum(char *, const char *);

/*
 * Functions
 */
int is_valid_ip_address(char const *ip_address)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip_address, &(sa.sin_addr));
    return result != 0;
}

int input_error(FILE * stream, char const *program_name)
{
    fprintf(stream, BOLD_ON UNDERLINED_ON RED
            "Error:" DEFAULT " see %s -h for the usage\n\n", program_name);
    exit(EXIT_FAILURE);
}

void print_time(long utime, const char *string, FILE * file)
{
    // printf("u_time: %ld\n", utime);

    double stime = (double) utime / 1000000;
    // int integral_stime = (int) stime;
    // double fractional_stime = (stime - integral_stime) * 1000000;
    // int fract_stime = (int) fractional_stime;
    // double mtime = stime / 60;
    // int integral_mtime = (int) mtime;
    // int fract_mtime = (int) integral_stime % 60;

    // printf("stime: %lf\n", stime);
    // printf("integral_stime: %d\n", integral_stime);
    // printf("fractional_stime: %d\n", fract_stime);
    // printf("mtime: %lf\n", mtime);
    // printf("integral_mtime: %d\n", integral_mtime);
    // printf("fractional_mtime: %d\n", fract_mtime);

    if (file != NULL)
        fprintf(file, "TIME %s:, %lf, seconds", string, stime);

    printf("TIME %s: %lf seconds\n", string, stime);
}

char *md5sum(char *buf, const char *filepath)
{
    char command[LEN];
    struct stat st = { 0 };

    sprintf((char *) command, "/usr/bin/md5sum");
    // printf("command: %s\n", command);

    if (stat(command, &st) == -1) {

        sprintf(buf, "command md5sum not found in '/usr/bin/', install it!\n");
        return "";

    } else {

        FILE *file;
        char command_with_file[LEN];

        sprintf((char *) command_with_file, "/usr/bin/md5sum %s", filepath);

        // abort_on_error(system(command_with_file) == -1, "system() error in common_func");

        /* Open the command for reading. */
        file = popen((const char *) command_with_file, "r");
        abort_on_error(file == NULL, "popen() error");

        /* Read the output a line at a time. */
        size_t size;
        abort_on_error(getline(&buf, &size, file) == -1, "getline() error in common_func");
        // printf("buf[%s]", buf);
        // while (fread(buf,sizeof(char), LINE_LENGTH, file))
        //  ;

        return buf;
    }
}

int connection_checker(struct timeval start, long timeout)
{
    struct timeval end;

    abort_on_error(gettimeofday(&end, NULL) == -1, "Error in gettimeofday() in timeout_checker");

    if (end.tv_sec - start.tv_sec > timeout) {
        return 1;
    } else {
        return 0;
    }

}
