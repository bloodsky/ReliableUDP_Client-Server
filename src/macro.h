/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Macro
 */

#ifndef MACRO_H
#define MACRO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define forever for(;;)

#define unused(x) (void)(x)

#define abort_on_error(cond, msg) do {			 		        \
	if (cond) { 											 	\
		fprintf(stderr, RED "%s (errno=%d [%s])\n" DEFAULT,  	\
						msg, errno, strerror(errno)); 			\
        exit(EXIT_FAILURE); 									\
	} 															\
} while (0)

#define abort_on_error_client_child(cond, msg) do {             \
    if (cond) {                                                 \
        fprintf(stderr, RED "%s (errno=%d [%s])\n" DEFAULT,     \
                        msg, errno, strerror(errno));           \
        printf("> ");                                           \
        fflush(stdout);                                         \
        exit(EXIT_FAILURE);                                     \
    }                                                           \
} while (0)

#define abort_on_pthread_error(cond, msg) abort_on_error((errno=cond), msg)

#define eprintf(format, ...) do {                 	\
    if (verbose)                              		\
        fprintf(stderr, format, ##__VA_ARGS__);   	\
} while (0)

#define vprintf(format, ...) do {                 	\
    if (verbose)                              		\
        printf(format, ##__VA_ARGS__);   			\
} while (0)

#define lprintf(file, format, ...) do {             \
    if (log_enabled)                              	\
        fprintf(file, format, ##__VA_ARGS__);   	\
} while (0)

#define tprintf(file, format, ...) do {             \
    if (for_test)                                   \
        fprintf(file, format, ##__VA_ARGS__);       \
} while (0)

#endif                          // MACRO_H
