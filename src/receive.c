/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Receiver Functions for Selective Repeat
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>

#include "macro.h"
#include "constants.h"
#include "packet.h"
#ifndef COMM_FUNC
#define COMM_FUNC "common_func.c"
#include COMM_FUNC
#endif
#ifndef PROB
#define PROB "prob.c"
#include PROB
#endif


// Prototypes
void recvfrom_rel(int, void *, size_t, struct sockaddr *, socklen_t *,
                  int, int, double, long, char *, unsigned long, int, int);
size_t deliver(PACK, FILE *);


// Variables
int verbose, for_test;
long num_packets_rcv = 0;
long filesize = 0, count = 1;
struct timeval recvfrom_timeout;


// Functions
void recvfrom_rel(int sockfd, void *buf, size_t len,
                  struct sockaddr *src_addr, socklen_t * addrlen,
                  int cmd_type, int win_len, double prob, long timeout,
                  char *filepath, unsigned long fs, int client, int prog_bar_en)
{
    unused(cmd_type);
    unused(timeout);
    unused(client);

    long received = 0;
    ssize_t n = 0;
    PACK packet;
    struct timeval conn_start;

    packet.seq_num = 0;
    packet.status = 0;
    memset(packet.data, 0, sizeof(packet.data));

    filesize = fs;

    //     recvfrom_timeout.tv_sec = END_CONNECTION_TIMEOUT;
    // abort_on_error(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &recvfrom_timeout, sizeof(recvfrom_timeout)) == -1, "setsockopt() in recvfrom_rel error");
    abort_on_error(fcntl(sockfd, F_SETFL, O_NONBLOCK),
                   "Error in fcntl(), could not set file flags");

    // srand((unsigned int) time(NULL));

    // if is command
    if (!strcmp(filepath, "")) {

        int last_acked = 0;
        abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                       "gettimeofday() in recvfrom_rel error");

        do {
            n = recvfrom(sockfd, &packet, sizeof(packet), 0, src_addr, addrlen);
            // abort_on_error(n == -1, "recvfrom() error in recvfrom_rel");
            abort_on_error(n == -1
                           && (errno != EAGAIN
                               || errno != EWOULDBLOCK), "recvfrom() command error");
            // abort_on_error(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT), "Other side of connection not responding... Try again!!!");
            // if (client && n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT)) {
            //  printf(RED "Other side of connection not responding... Try again!!!" DEFAULT "\n");
            //  if (client){
            //      printf("> ");
            //      fflush(stdout);
            //  }
            //  exit(EXIT_FAILURE);
            // }

            if (n > 0 && packet.status == COMMAND) {
                vprintf("Received packet #: %lu\n", packet.seq_num);
                // vprintf("Packet Data: [%s]\n", packet.data);

                abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                               "gettimeofday() in recvfrom_rel error");

                memcpy(buf, packet.data, len);

                // memset(packet.data, 0, len);
                packet.status = ACK;

                if (execute_with_prob(prob)) {
                    vprintf("SENT PACKET ACK\n");
                    last_acked = 1;
                    n = sendto(sockfd, &packet, sizeof(packet), 0, src_addr, *addrlen);
                    abort_on_error(n == -1
                                   && (errno != EAGAIN
                                       || errno != EWOULDBLOCK),
                                   "sendto() ack error in recvfrom_rel");
                    // if (errno == EAGAIN || errno == EWOULDBLOCK)
                    //  usleep(250);
                } else {
                    vprintf("NOT SENT PACKET ACK\n");
                }
                received += n;
            }
        } while (!last_acked);

        // else is data
    } else {

        if (for_test) {
            char filename[100] = "";
            if (timeout != -1) {
                sprintf(filename, "%s_%d_%.1f_%ld_received.log", filepath, win_len, prob, timeout);
            } else {
                sprintf(filename, "%s_%d_%.1f_a_received.log", filepath, win_len, prob);
            }
            f = fopen(filename, "w+");
            abort_on_error(f == NULL, "fopen() error");
        }

        int last_received = 0, all_acked = 0;
        long writed = 0, base = 0;
        PACK buffer[win_len];
        int sent_ack[win_len];
        FILE *file;
        float progress = 0.0;

        if (filesize % MAX_PK_DATA_SIZE == 0)
            num_packets_rcv = filesize / MAX_PK_DATA_SIZE;
        else
            num_packets_rcv = (filesize / MAX_PK_DATA_SIZE) + 1;
        // to_receive = num_packets_rcv * sizeof(struct pkt);

        for (int i = 0; i < win_len; ++i) {
            buffer[i].status = NODATA;
        }
        for (int i = 0; i < win_len; ++i) {
            sent_ack[i] = 0;
        }

        file = fopen(filepath, "w+");
        abort_on_error(file == NULL, "fopen() file error in recvfrom_rel");

        // for test
        // abort_on_error(1, "ERROR TEST");

        abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                       "gettimeofday() in recvfrom_rel error");

        forever {
            n = recvfrom(sockfd, &packet, sizeof(packet), 0, src_addr, addrlen);
            // abort_on_error(n == -1, "recvfrom() data error in recvfrom_rel");
            abort_on_error(n == -1
                           && (errno != EAGAIN || errno != EWOULDBLOCK), "recvfrom() data error");
            // abort_on_error(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT), "Other side of connection not responding... Try again!!!");
            //       if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT)) {
            //  printf(RED "\nOther side of connection not responding... Try again!!!" DEFAULT "\n");
            //  if (client){
            //      printf("> ");
            //      fflush(stdout);
            //  }
            //  exit(EXIT_FAILURE);
            // }

            if (n > 0) {
                vprintf("Pack#: %lu\n", packet.seq_num);

                abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                               "gettimeofday() in recvfrom_rel error");

                long sq = packet.seq_num;

                // vprintf("Packet.status: %d\n", packet.status);
                // vprintf("Packet.data: [%s]\n", packet.data);
                // vprintf("num_packets_rcv: %ld - count: %ld\n", num_packets_rcv, count);

                // vprintf("base: %ld - sq: %d - last_delivered: %d\n", base, sq, last_delivered);
                tprintf(f, "base: %ld - sq: %ld --> ", base, sq);

                if ((long) buffer[sq % win_len].seq_num != sq) {
                    sent_ack[sq % win_len] = 0;
                }

                if (sq < base) {    // packet already received; resend ack
                    vprintf("Packet %ld received: already delivered!\n", sq);
                    tprintf(f, "Packet %ld received: already delivered! -> ", sq);

                    packet.status = ACK;

                    if (execute_with_prob(prob)) {
                        sent_ack[sq % win_len] = 1;
                        vprintf("SENT PACKET %ld ACK\n", sq);
                        tprintf(f, "sent ack: %d\n", sent_ack[sq % win_len]);
                        n = sendto(sockfd, &packet, sizeof(packet), 0, src_addr, *addrlen);
                        abort_on_error(n == -1
                                       && (errno != EAGAIN
                                           || errno != EWOULDBLOCK),
                                       "sendto() file ack error in recvfrom_rel");
                        // if (errno == EAGAIN || errno == EWOULDBLOCK)
                        //  usleep(250);
                    } else {
                        vprintf("NOT SENT PACKET %ld ACK\n", sq);
                        tprintf(f, "not sent ack: %d\n", sent_ack[sq % win_len]);
                    }

                } else if (sq >= base && sq < base + win_len) {
                    vprintf("Packet %ld received: send ACK!\n", sq);
                    tprintf(f, "Packet %ld received -> ", sq);

                    packet.status = ACK;
                    buffer[sq % win_len] = packet;

                    if (execute_with_prob(prob)) {
                        sent_ack[sq % win_len] = 1;
                        vprintf("SENT PACKET %ld ACK\n", sq);
                        tprintf(f, "sent ack: %d\n", sent_ack[sq % win_len]);
                        n = sendto(sockfd, &packet, sizeof(packet), 0, src_addr, *addrlen);
                        abort_on_error(n == -1
                                       && (errno != EAGAIN
                                           || errno != EWOULDBLOCK),
                                       "sendto() file ack error in recvfrom_rel");
                        // if (errno == EAGAIN || errno == EWOULDBLOCK)
                        //  usleep(250);
                    } else {
                        vprintf("NOT SENT PACKET %ld ACK\n", sq);
                        tprintf(f, "not sent ack: %d\n", sent_ack[sq % win_len]);
                    }

                    if (sq == base) {   // first packet waiting for
                        vprintf("Deliver Packet %ld\n", base);
                        tprintf(f, "Deliver Packet %ld\n", base);
                        writed += deliver(buffer[sq % win_len], file);
                        buffer[sq % win_len].status = NODATA;
                        // sent_ack[sq%win_len] = 0;

                        // check if some more packets can be delivered
                        int i;
                        for (i = 1; sq + i < base + win_len; i++) {
                            if (buffer[(sq + i) % win_len].status == NODATA) {
                                break;
                            } else {
                                vprintf("Deliver Packet %ld also\n", sq + i);
                                tprintf(f, "Deliver Packet %ld also\n", sq + i);
                                writed += deliver(buffer[(sq + i) % win_len], file);
                                buffer[(sq + i) % win_len].status = NODATA;
                                // sent_ack[sq%win_len] = 0;
                            }
                        }

                        base += i;  // move base

                        // progress bar
                        if (prog_bar_en && !verbose) {
                            int barWidth = 50;

                            progress = base * 100 / num_packets_rcv;

                            printf(GREEN "[");
                            int pos = barWidth * progress / 100;

                            for (int i = 0; i <= barWidth; i++) {
                                if (i < pos)
                                    printf("=");
                                else if (i == pos)
                                    printf(">");
                                else
                                    printf(" ");
                            }

                            printf("] %d%%\r" DEFAULT, (int) (progress));
                            fflush(stdout);
                        }
                    }
                } else {
                    vprintf("Packet %ld received: ignore!\n", sq);
                    tprintf(f, "Packet %ld received: ignore!\n", sq);
                    // usleep(10000); // necessary!
                }

                if (sq + 1 == num_packets_rcv) {
                    last_received = 1;
                    // vprintf("TEST ENDING BUFFER - Last packet received\n");
                    // lprintf(f, "TEST ENDING BUFFER - Last packet received\n");
                    if (num_packets_rcv < win_len) {
                        for (int i = 0; i < num_packets_rcv; ++i) {
                            // vprintf("POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            // lprintf(f, "POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            if (!sent_ack[i]) {
                                all_acked = 0;
                                break;
                            } else {
                                all_acked = 1;
                            }
                        }
                    } else {
                        for (int i = 0; i < win_len; ++i) {
                            // vprintf("POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            // lprintf(f, "POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            if (!sent_ack[i]) {
                                all_acked = 0;
                                break;
                            } else {
                                all_acked = 1;
                            }
                        }
                    }
                }

                if (last_received) {
                    // vprintf("TEST ENDING BUFFER - not last packet, but last packet received\n");
                    // lprintf(f, "TEST ENDING BUFFER - not last packet, but last packet received\n");
                    if (num_packets_rcv < win_len) {
                        for (int i = 0; i < num_packets_rcv; ++i) {
                            // vprintf("POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            // lprintf(f, "POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            if (!sent_ack[i]) {
                                all_acked = 0;
                                break;
                            } else {
                                all_acked = 1;
                            }
                        }
                    } else {
                        for (int i = 0; i < win_len; ++i) {
                            // vprintf("POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            // lprintf(f, "POS %d -> packet #%ld, sent_ack: %d.\n", i, buffer[i].seq_num, sent_ack[i]);
                            if (!sent_ack[i]) {
                                all_acked = 0;
                                break;
                            } else {
                                all_acked = 1;
                            }
                        }
                    }
                }
                received += n;

                if (base + 1 >= num_packets_rcv && last_received && all_acked) {
                    vprintf("Last packet received and all packets acked: ENDING...\n");
                    tprintf(f, "Last packet received and all packets acked: ENDING...\n");
                    break;
                }

                if (for_test)
                    fflush(f);
            }
        }

        vprintf("Tot Bytes received: %ld\n", received);
        vprintf("Tot Bytes writed: %ld\n", writed);
        tprintf(f, "Tot Bytes received: %ld\n", received);
        tprintf(f, "Tot Bytes writed: %ld\n", writed);

        abort_on_error(fclose(file) == EOF, "fclose() file error in recvfrom_rel");
    }
}

size_t deliver(PACK packet, FILE * file)
{

    int last_size = filesize - ((num_packets_rcv - 1) * MAX_PK_DATA_SIZE);
    int w = 0;

    if (num_packets_rcv == 1) {
        w = fwrite(packet.data, sizeof(char), filesize, file);
        vprintf(GREEN "writed packed %lu\n" DEFAULT, packet.seq_num);
        tprintf(f, "writed packed %lu\n", packet.seq_num);
    } else {
        if (count < num_packets_rcv) {
            w = fwrite(packet.data, sizeof(char), MAX_PK_DATA_SIZE, file);
            vprintf(GREEN "writed packed %lu\n" DEFAULT, packet.seq_num);
            tprintf(f, "writed packed %lu\n", packet.seq_num);
        } else {
            if (filesize % MAX_PK_DATA_SIZE == 0) {
                w = fwrite(packet.data, sizeof(char), MAX_PK_DATA_SIZE, file);
                vprintf(GREEN "writed packed %lu\n" DEFAULT, packet.seq_num);
                tprintf(f, "writed packed %lu\n", packet.seq_num);
            } else {
                w = fwrite(packet.data, sizeof(char), last_size, file);
                vprintf(GREEN "writed packed %lu\n" DEFAULT, packet.seq_num);
                tprintf(f, "writed packed %lu\n", packet.seq_num);
            }
        }
    }

    count++;

    return w;
}
