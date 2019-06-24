/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Files Input/Output Functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#include "constants.h"

/*
 * Prototypes
 */
void write_list(char *, FILE *);
void print_list(FILE *);
char *createpath(char *, int);


void write_list(char *directory, FILE * file)
{
    struct dirent **namelist;
    int n;

    n = scandir(directory, &namelist, NULL, alphasort);
    if (n == -1)
        perror("scandir()");
    else {
        int i = 0;
        while (i < n) {

            if (namelist[i]->d_type != DT_DIR) {
                // printf(BLUE "%s\n" DEFAULT, namelist[i]->d_name);
                // write(fd, (unsigned char *) BLUE, strlen(BLUE));
                // write(fd, (unsigned char *) namelist[i]->d_name, strlen(namelist[i]->d_name));
                // write(fd, (unsigned char *) DEFAULT, strlen(DEFAULT));
                // write(fd, (unsigned char *) "\n\0", 1);
                fwrite(BLUE, sizeof(char), strlen(BLUE), file);
                fwrite(namelist[i]->d_name, sizeof(char), strlen(namelist[i]->d_name), file);
                fwrite(DEFAULT, sizeof(char), strlen(DEFAULT), file);
                fwrite("\n\0", sizeof(char), 1, file);
            } else {
                // printf(GREEN "%s\n" DEFAULT, namelist[i]->d_name);
                // write(fd, (unsigned char *) GREEN, strlen(GREEN));
                // write(fd, (unsigned char *) namelist[i]->d_name, strlen(namelist[i]->d_name));
                // write(fd, (unsigned char *) DEFAULT, strlen(DEFAULT));
                // write(fd, (unsigned char *) "\n\0", 1);
                fwrite(GREEN, sizeof(char), strlen(GREEN), file);
                fwrite(namelist[i]->d_name, sizeof(char), strlen(namelist[i]->d_name), file);
                fwrite(DEFAULT, sizeof(char), strlen(DEFAULT), file);
                fwrite("\n\0", sizeof(char), 1, file);
            }

            // printf("%s\n", namelist[i]->d_name);
            free(namelist[i]);
            ++i;
        }
        free(namelist);
    }
}

void print_list(FILE * list)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, list)) != -1) {
        // printf("Retrieved line of length %zu :\n", read);
        printf("%s", line);
    }
}

char *createpath(char *fname, int type)
{
    // int filepathlen;
    // filepathlen = 6 + strlen(fname);
    char *fpath = calloc(LEN, sizeof(char));

    if (type == 0) {
        strcat(fpath, CFILES);
    } else if (type == 1) {
        strcat(fpath, SFILES);
    }

    strcat(fpath, fname);
    strcat(fpath, DEL);
    return fpath;
}
