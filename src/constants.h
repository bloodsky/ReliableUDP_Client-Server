/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Constants
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BOLD_ON 		"\033[1m"
#define UNDERLINED_ON  	"\033[4m"
#define YELLOW  		"\033[33m"
#define RED 			"\033[31m"
#define GREEN 			"\x1B[32m"
#define BLUE 			"\x1B[34m"
#define DEFAULT 		"\033[0m"

#define LEN 200
#define IN_BUFF 100
#define MAXLEN 1024

#define LINE_LENGTH 1035

#define MAX_PK_DATA_SIZE 1400

#define MAX_WIN_LEN 100

#define INIT_ADPT_TIMEOUT 500000    // 0.5 seconds
#define ALPHA 0.125
#define BETHA 0.25

#define END_CONNECTION_TIMEOUT 30

#define COMMAND (unsigned int) 0
#define NODATA (unsigned int) -1
#define DATA_BUFFERED (unsigned int) 2
#define ACK (unsigned int) 1
#define ACKED (unsigned int) 3

#define GET "get"
#define PUT "put"
#define LIST "list"
#define HELP "help"
#define PRINT "print"
#define EXIT "exit"
#define NL "\n"
#define SPACE " "
#define DEL "\0"
#define SEP ","
#define SPACE_LOG "      "
#define END_FLAG "================END"
#define FNE_FLAG "File doesn't exist"
#define OK_FLAG "OK"

#define CFILES "ClientFiles/"
#define SFILES "ServerFiles/"
#define LCFILES "/tmp/clist"
#define LSFILES "/tmp/slist"
#define LCRFILES "/tmp/crlist"

#endif                          // CONSTANTS_H
