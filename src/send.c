/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Sender Functions for Selective Repeat
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>

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
void sendto_rel(int, const void *, size_t, const struct sockaddr *, socklen_t,
                int, int, double, long, char *, long, int, int);
void *sender(void *);
void *ack_checker(void *);
int timeout_checker(int);
long calc_adpt_timeout(int);


// Threads Variables
pthread_t sender_thread;
pthread_t ack_checker_thread;
// pthread_t timeout_checker_thread;

// Variables
int verbose, for_test;
int threads_started = 0;
int ack_checker_quitted = 0;
long num_packets = 0;
long ack_count = 0, sent = 0, to_send = 0, readed = 0;
char *filepath;
FILE *file, *f, *ftime;
int socketfd;
const struct sockaddr *destaddr;
socklen_t destaddr_len;
int cmd_type;
long timeout;
double probability;
int winlen;
int position = 0;
long base = 0;
int client = 0;
int progress_bar_enabled = 0;

// Mutex variables
pthread_mutexattr_t mtx_attr;
pthread_mutex_t mtx;
pthread_cond_t cnd_not_full;

// Packets variables
PACK packet_snd;
PACK packet_rcv;
PACK pkts[MAX_WIN_LEN];
int retransmitted[MAX_WIN_LEN];

// Timeout Variables
int adaptive = 0;
long long measurement = 0;
struct timeval t_start[MAX_WIN_LEN];
struct timeval t_end[MAX_WIN_LEN];
struct timeval t_adpt_end[MAX_WIN_LEN];
long estimated_RTT;
long dev_RTT;


// Functions
void sendto_rel(int sockfd, const void *buf, size_t len,
                const struct sockaddr *dest_addr, socklen_t addrlen,
                int cmdtype, int win_len, double prob, long tmt,
                char *path, long filesize, int cli, int prog_bar_en)
{
    cmd_type = cmdtype;
    filepath = path;
    destaddr = dest_addr;
    destaddr_len = addrlen;
    socketfd = sockfd;
    probability = prob;
    winlen = win_len;
    if (tmt == -1) {
        adaptive = 1;
        for (int i = 0; i < winlen; ++i) {
            t_adpt_end[i].tv_sec = 0;
            t_adpt_end[i].tv_usec = 0;
        }
        estimated_RTT = 0;
        dev_RTT = 0;
        // timeout = INIT_ADPT_TIMEOUT;
    }
    // else
    //  timeout = tmt;
    timeout = INIT_ADPT_TIMEOUT;

    client = cli;
    progress_bar_enabled = prog_bar_en;

    packet_snd.seq_num = NODATA;
    packet_rcv.seq_num = NODATA;
    memset(packet_snd.data, 0, sizeof(packet_snd.data));
    memset(packet_rcv.data, 0, sizeof(packet_rcv.data));

    // abort_on_pthread_error(pthread_attr_init(NULL), "Error in pthread_attr_init()");
    // abort_on_pthread_error(pthread_attr_setdetachstate(NULL, PTHREAD_CREATE_DETACHED), "Error in pthread_attr_setdetachstate()");

    abort_on_pthread_error(pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_ERRORCHECK),
                           "Error in pthread_mutexattr_settype()");

    abort_on_pthread_error(pthread_mutex_init(&mtx, &mtx_attr), "Error in pthread_mutex_init()");
    abort_on_pthread_error(pthread_cond_init(&cnd_not_full, NULL), "Error in pthread_cond_init()");

    abort_on_error(fcntl(socketfd, F_SETFL, O_NONBLOCK),
                   "Error in fcntl(), could not set file flags");

    // srand((unsigned int) time(NULL));

    // if is command
    if (!strcmp(filepath, "")) {

        memcpy(packet_snd.data, buf, len);

        abort_on_pthread_error(pthread_create(&sender_thread, NULL, sender, NULL),
                               "Creating sender failed");

        while (!threads_started) {
            usleep(100);
        }

        abort_on_pthread_error(pthread_join(sender_thread, NULL), "Joining sender failed");

        // else is data
    } else {

        if (filesize % MAX_PK_DATA_SIZE == 0)
            num_packets = filesize / MAX_PK_DATA_SIZE;
        else
            num_packets = (filesize / MAX_PK_DATA_SIZE) + 1;
        to_send = num_packets * sizeof(struct pkt);

        for (int i = 0; i < winlen; ++i) {
            pkts[i].status = NODATA;
        }

        if (!adaptive)
            timeout = tmt;

        abort_on_pthread_error(pthread_create(&sender_thread, NULL, sender, NULL),
                               "Creating reader failed");

        while (!threads_started) {
            usleep(100);
        }

        abort_on_pthread_error(pthread_join(sender_thread, NULL), "Joining sender failed");
    }
}

void *sender(void *arg)
{
    unused(arg);
    ssize_t n = 0;
    threads_started = 0;
    sent = 0;

    vprintf(YELLOW "sender thread running...\n" DEFAULT);

    // if is command
    if (!strcmp(filepath, "")) {

        packet_snd.seq_num = rand_seq_num();
        packet_snd.status = COMMAND;

        if (execute_with_prob(probability)) {

            vprintf(YELLOW "EXECUTED SENDTO() in SENDER()\n" DEFAULT);
            n = sendto(socketfd, &packet_snd, sizeof(struct pkt), 0, destaddr, destaddr_len);
            abort_on_error(n == -1
                           && (errno != EAGAIN
                               || errno != EWOULDBLOCK), "sendto() command in sender error");

        } else {
            vprintf(YELLOW "NOT EXECUTED SENDTO() in SENDER()\n" DEFAULT);
        }
        // start timeout
        abort_on_error(gettimeofday(&t_start[0], NULL) == -1, "gettimeofday() in sender error");
        vprintf(BLUE "Timeout for packet started: %ld\n" DEFAULT,
                t_start[0].tv_sec * 1000000 + t_start[0].tv_usec);

        // start ack_checker
        abort_on_pthread_error(pthread_create(&ack_checker_thread, NULL, ack_checker, NULL),
                               "Creating ack_checker failed");
        threads_started++;

        // else is data
    } else {

        if (for_test) {
            char filename[100] = "";
            if (!adaptive) {
                sprintf(filename, "%s_%d_%.1f_%ld_sent.log", filepath, winlen, probability,
                        timeout);
            } else {
                sprintf(filename, "%s_%d_%.1f_a_sent.log", filepath, winlen, probability);
            }
            f = fopen(filename, "w+");
            abort_on_error(f == NULL, "fopen() file for-test error");
        }

        if (for_test && adaptive) {
            char filename2[100] = "";
            sprintf(filename2, "%s_%d_%.1f_a_adaptive_timeout.log", filepath, winlen, probability);
            ftime = fopen(filename2, "w+");
            abort_on_error(ftime == NULL, "fopen() file time for-test error");
            tprintf(ftime, "START, END, sample_RTT, estimated_RTT, dev_RTT, timeout\n");
            fflush(ftime);
        }

        float progress = 0.0;
        long nextseqnum = 0;
        size_t r = 1;

        file = fopen(filepath, "r");
        abort_on_error(file == NULL, "fopen() file error in sender");

        forever {

            // for test
            // abort_on_error(1, "ERROR TEST");

            abort_on_pthread_error(pthread_mutex_lock(&mtx),
                                   "pthread_mutex_lock() in sender error");

            if (nextseqnum < base + winlen) {

                r = fread(pkts[nextseqnum % winlen].data, sizeof(char), MAX_PK_DATA_SIZE, file);
                abort_on_error(ferror(file), "fread() file error");

                if (!r) {
                    abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                           "pthread_mutex_unlock() in sender error");
                    break;
                }

                readed += r;
                pkts[nextseqnum % winlen].seq_num = nextseqnum;
                pkts[nextseqnum % winlen].status = DATA_BUFFERED;
                retransmitted[nextseqnum % winlen] = 0;

                // printf("nextseqnum%%winlen: %ld\n", nextseqnum%winlen);

                // vprintf("PKT#%ld -> data: [%s]\n", pkts[nextseqnum%winlen].seq_num, pkts[nextseqnum%winlen].data);
                tprintf(f, "PKT #%ld with status: %d @position %ld -> ",
                        pkts[nextseqnum % winlen].seq_num, pkts[nextseqnum % winlen].status,
                        nextseqnum % winlen);

                if (execute_with_prob(probability)) {
                    vprintf(YELLOW "EXECUTED SENDTO() PACK %ld in SENDER() @position: %ld\n"
                            DEFAULT, nextseqnum, (nextseqnum % winlen));
                    tprintf(f, "sent\n");
                    n = sendto(socketfd, &(pkts[nextseqnum % winlen]), sizeof(struct pkt), 0,
                               destaddr, destaddr_len);
                    abort_on_error(n == -1
                                   && (errno != EAGAIN
                                       || errno != EWOULDBLOCK), "sendto() file in sender error");
                    sent += n;
                } else {
                    vprintf(YELLOW "NOT EXECUTED SENDTO() PACK %ld in SENDER() @position: %ld\n"
                            DEFAULT, nextseqnum, (nextseqnum % winlen));
                    tprintf(f, "not sent\n");
                }
                // start timeout
                abort_on_error(gettimeofday(&t_start[nextseqnum % winlen], NULL) == -1,
                               "gettimeofday() in sender error");
                vprintf(BLUE "Timeout for packet %ld started: %ld\n" DEFAULT, nextseqnum,
                        t_start[nextseqnum % winlen].tv_sec * 1000000 +
                        t_start[nextseqnum % winlen].tv_usec);

                // start ack_checker
                if (!threads_started) {
                    abort_on_pthread_error(pthread_create
                                           (&ack_checker_thread, NULL, ack_checker, NULL),
                                           "Creating ack_checker in sender failed");
                    threads_started++;
                }

                nextseqnum++;

                // progress bar
                if (progress_bar_enabled && !verbose) {
                    int barWidth = 50;

                    progress = nextseqnum * 100 / num_packets;

                    // printf("progress: %f, step: %ld\n", progress, base);

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

            } else {
                /* buffer full */
                vprintf(YELLOW "buffer full: Refused Packet!\n" DEFAULT);
                tprintf(f, "buffer full: Refused Packet!\n");
                abort_on_pthread_error(pthread_cond_wait(&cnd_not_full, &mtx),
                                       "pthread_cond_wait() in sender error");
            }

            abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                   "pthread_mutex_unlock() in sender error");
        }

        abort_on_error(fclose(file) == EOF, "fclose() file error in sender");
    }

    // if (log_enabled)
    //  abort_on_error(fclose(f) == EOF, "fclose() f error in sender");
    // if (log_enabled && adaptive)
    //  abort_on_error(fclose(ftime) == EOF, "fclose() file error in sender");

    abort_on_pthread_error(pthread_join(ack_checker_thread, NULL), "Joining ack_checker failed");
    // vprintf(YELLOW "sender thread quitting now...\n" DEFAULT);
    pthread_exit(NULL);
}

void *ack_checker(void *arg)
{
    unused(arg);
    ssize_t n = 0;
    ack_checker_quitted = 0;
    struct timeval conn_start;

    vprintf(GREEN "ack_checker thread running...\n" DEFAULT);

    // if is command
    if (!strcmp(filepath, "")) {

        abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                       "gettimeofday() in ack_checker error");

        forever {

            abort_on_pthread_error(pthread_mutex_lock(&mtx),
                                   "pthread_mutex_lock() in ack_checker error");

            n = recvfrom(socketfd, &(packet_rcv), sizeof(struct pkt), 0,
                         (struct sockaddr *) destaddr, &destaddr_len);
            abort_on_error(n == -1
                           && (errno != EAGAIN
                               || errno != EWOULDBLOCK),
                           "recvfrom() command ack in ack_checker error");
            // abort_on_error(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT), "Other side of connection not responding... Try again!!!");
            // if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT)) {
            //  printf(RED "\nOther side of connection not responding... Try again!!!" DEFAULT "\n");
            //  abort_on_pthread_error(pthread_cancel(sender_thread), "pthread_cancel() in ack_checker failed");
            //  if (client) {
            //      printf("> ");
            //      fflush(stdout);
            //  }
            //        pthread_exit(NULL);
            // }

            // vprintf("packet_rcv.seq_num: %lu - packet_snd.seq_num: %lu\n", packet_rcv.seq_num, packet_snd.seq_num);
            if (n && (packet_snd.seq_num == packet_rcv.seq_num) && (packet_rcv.status == ACK)) {

                abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                               "gettimeofday() in recvfrom_rel error");

                packet_rcv.status = ACKED;
                vprintf(GREEN "PACKET %lu ACKED!!!\n" DEFAULT, packet_rcv.seq_num);
                // packet_rcv.seq_num = NODATA;
                sent = n;
                ack_checker_quitted = 1;
                abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                       "pthread_mutex_unlock() in ack_checker error");
                break;
            }

            if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && timeout_checker(0)) {

                vprintf(BLUE "Timeout elapsed for packet %lu! RESENDING...\n" DEFAULT,
                        packet_snd.seq_num);

                if (execute_with_prob(probability)) {
                    vprintf(GREEN "EXECUTED SENDTO() in ACK_CHECKER()\n" DEFAULT);
                    n = sendto(socketfd, &packet_snd, sizeof(struct pkt), 0, destaddr,
                               destaddr_len);
                    abort_on_error(n == -1
                                   && (errno != EAGAIN
                                       || errno != EWOULDBLOCK), "sendto() in ack_checker error");
                    // if (errno == EAGAIN || errno == EWOULDBLOCK)
                    //  usleep(250);
                } else {
                    vprintf(GREEN "NOT EXECUTED SENDTO() in ACK_CHECKER()\n" DEFAULT);
                }
                // restart timeout
                abort_on_error(gettimeofday(&t_start[0], NULL) == -1,
                               "gettimeofday() in sender error");
                vprintf(BLUE "Timeout for packet started: %ld\n" DEFAULT,
                        t_start[0].tv_sec * 1000000 + t_start[0].tv_usec);
            }
            abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                   "pthread_mutex_unlock() in ack_checker error");
        }

        // else is data
    } else {

        abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                       "gettimeofday() in ack_checker error");

        // float progress = 0.0;

        // while (!ack_checker_quitted) {
        forever {

            long sq = 0;
            packet_rcv.status = NODATA;
            n = 0;

            abort_on_pthread_error(pthread_mutex_lock(&mtx),
                                   "pthread_mutex_lock() in ack_checker error");

            n = recvfrom(socketfd, &(packet_rcv), sizeof(struct pkt), 0,
                         (struct sockaddr *) destaddr, &destaddr_len);
            abort_on_error(n == -1
                           && (errno != EAGAIN
                               || errno != EWOULDBLOCK),
                           "recvfrom() data ack in ack_checker error");
            // abort_on_error(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT), "Other side of connection not responding... Try again!!!");
            // if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && connection_checker(conn_start, END_CONNECTION_TIMEOUT)) {
            //  printf(RED "\nOther side of connection not responding... Try again!!!" DEFAULT "\n");
            //  abort_on_pthread_error(pthread_cancel(sender_thread), "pthread_cancel() in ack_checker failed");
            //  if (client) {
            //      printf("> ");
            //      fflush(stdout);
            //  }
            //        pthread_exit(NULL);
            // }

            sq = packet_rcv.seq_num;

            // if (n > 0) {
            // lprintf(f, "Received PACK #%ld -> data: %s!!!\n", sq, packet_rcv.data);
            // }

            if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {

                if (num_packets < winlen) {

                    for (int i = 0; i < num_packets; ++i) {

                        if (timeout_checker(i) && pkts[i].status != ACKED) {

                            retransmitted[i] = 1;

                            // vprintf("PKT#%ld -> data: [%s]\n", pkts[i].seq_num, pkts[i].data);

                            vprintf(BLUE "Timeout elapsed for packet %lu! RESENDING...\n" DEFAULT,
                                    pkts[i].seq_num);
                            tprintf(f, "Timeout elapsed for packet %lu! RESENDING... -> ",
                                    pkts[i].seq_num);

                            if (execute_with_prob(probability)) {
                                vprintf(GREEN
                                        "EXECUTED SENDTO() PACK %ld in ACK_CHECKER() @position: %d\n"
                                        DEFAULT, pkts[i].seq_num, i);
                                tprintf(f, "sent!\n");
                                n = sendto(socketfd, &(pkts[i]), sizeof(struct pkt), 0, destaddr,
                                           destaddr_len);
                                abort_on_error(n == -1
                                               && (errno != EAGAIN
                                                   || errno != EWOULDBLOCK),
                                               "sendto() file in sender error");
                                sent += n;
                            } else {
                                vprintf(GREEN
                                        "NOT EXECUTED SENDTO() PACK %ld in ACK_CHECKER() @position: %d\n"
                                        DEFAULT, pkts[i].seq_num, i);
                                tprintf(f, "not sent!\n");
                            }
                            // restart timeout
                            abort_on_error(gettimeofday(&t_start[i], NULL) == -1,
                                           "gettimeofday() in sender error");
                            vprintf(BLUE "Timeout for packet %ld started: %ld\n" DEFAULT,
                                    pkts[i].seq_num,
                                    t_start[i].tv_sec * 1000000 + t_start[i].tv_usec);
                        }
                    }

                } else {

                    for (int i = 0; i < winlen; ++i) {

                        if (timeout_checker(i) && pkts[i].status != ACKED) {

                            retransmitted[i] = 1;

                            // vprintf("PKT#%ld -> data: [%s]\n", pkts[i].seq_num, pkts[i].data);

                            vprintf(BLUE "Timeout elapsed for packet %lu! RESENDING...\n" DEFAULT,
                                    pkts[i].seq_num);
                            tprintf(f, "Timeout elapsed for packet %lu! RESENDING... -> ",
                                    pkts[i].seq_num);

                            if (execute_with_prob(probability)) {
                                vprintf(GREEN
                                        "EXECUTED SENDTO() PACK %ld in ACK_CHECKER() @position: %d\n"
                                        DEFAULT, pkts[i].seq_num, i);
                                tprintf(f, "sent!\n");
                                n = sendto(socketfd, &(pkts[i]), sizeof(struct pkt), 0, destaddr,
                                           destaddr_len);
                                abort_on_error(n == -1
                                               && (errno != EAGAIN
                                                   || errno != EWOULDBLOCK),
                                               "sendto() file in sender error");
                                sent += n;
                            } else {
                                vprintf(GREEN
                                        "NOT EXECUTED SENDTO() PACK %ld in ACK_CHECKER() @position: %d\n"
                                        DEFAULT, pkts[i].seq_num, i);
                                tprintf(f, "not sent!\n");
                            }
                            // restart timeout
                            abort_on_error(gettimeofday(&t_start[i], NULL) == -1,
                                           "gettimeofday() in sender error");
                            vprintf(BLUE "Timeout for packet %ld started: %ld\n" DEFAULT,
                                    pkts[i].seq_num,
                                    t_start[i].tv_sec * 1000000 + t_start[i].tv_usec);
                        }
                    }
                }
            }

            if (for_test)
                fflush(f);

            if (n > 0 && sq >= base && sq < base + winlen) {

                abort_on_error(gettimeofday(&conn_start, NULL) == -1,
                               "gettimeofday() in recvfrom_rel error");

                // long sq = packet_rcv.seq_num;
                // vprintf(GREEN "n: %ld - Received pkt: %ld\n" DEFAULT, n, sq);
                // lprintf(f, "n: %ld - Received pkt: %ld\n", n, sq);
                // vprintf("data: [%s]\n", packet_rcv.data);

                if (pkts[sq % winlen].status != ACKED) {

                    // mark packet as confirmed
                    pkts[sq % winlen].status = ACKED;
                    vprintf(GREEN "PACKET %lu ACKED!!!\n" DEFAULT, pkts[sq % winlen].seq_num);
                    tprintf(f, "PACKET %lu ACKED!!! with status:%d @position: %ld -> ",
                            pkts[sq % winlen].seq_num, pkts[sq % winlen].status, sq % winlen);

                    // calculating adaptive timeout
                    if (adaptive && !retransmitted[sq % winlen]) {
                        abort_on_error(gettimeofday(&t_adpt_end[sq % winlen], NULL) == -1,
                                       "Error in gettimeofday() in ack_checker");
                        timeout = calc_adpt_timeout(sq % winlen);
                    }
                    // vprintf("sq: %ld - base: %ld\n", sq, base);
                    tprintf(f, "sq: %ld - base: %ld\n", sq, base);

                    if (sq == base) {

                        int riseBase = 1;

                        if ((long) pkts[sq % winlen].seq_num + 1 == num_packets) {
                            vprintf("Tot Bytes readed: %ld\n", readed);
                            vprintf("Tot Bytes sent: %ld\n", sent);
                            tprintf(f, "Tot Bytes readed: %ld\n", readed);
                            tprintf(f, "Tot Bytes sent: %ld\n", sent);
                            ack_checker_quitted = 1;
                            abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                                   "pthread_mutex_unlock() in ack_checker error");
                            vprintf(GREEN "ack_checker quitting now...\n" DEFAULT);;
                            vprintf(GREEN "canceling sender thread now...\n" DEFAULT);
                            abort_on_pthread_error(pthread_cancel(sender_thread),
                                                   "pthread_cancel() in ack_checker failed");
                            pthread_exit(NULL);
                        }

                        if (num_packets < winlen) {

                            for (int i = 1; i < num_packets; i++) {
                                // vprintf("pkt# %ld -> status: %d\n", pkts[(sq+i)%winlen].seq_num, pkts[(sq+i)%winlen].status);
                                // lprintf(f, "pkt# %ld -> status: %d\n", pkts[(sq+i)%winlen].seq_num, pkts[(sq+i)%winlen].status);
                                if (pkts[(sq + i) % winlen].status == ACKED) {
                                    riseBase++;
                                    pkts[(sq + i) % winlen].status = NODATA;
                                    t_adpt_end[(sq + i) % winlen].tv_sec = 0;
                                    t_adpt_end[(sq + i) % winlen].tv_usec = 0;
                                    if ((long) pkts[(sq + i) % winlen].seq_num + 1 == num_packets) {
                                        vprintf("Tot Bytes readed: %ld\n", readed);
                                        vprintf("Tot Bytes sent: %ld\n", sent);
                                        tprintf(f, "Tot Bytes readed: %ld\n", readed);
                                        tprintf(f, "Tot Bytes sent: %ld\n", sent);
                                        ack_checker_quitted = 1;
                                        abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                                               "pthread_mutex_unlock() in ack_checker error");
                                        vprintf(GREEN "ack_checker quitting now...\n" DEFAULT);
                                        vprintf(GREEN "canceling sender thread now...\n" DEFAULT);
                                        abort_on_pthread_error(pthread_cancel(sender_thread),
                                                               "pthread_cancel() in ack_checker failed");
                                        pthread_exit(NULL);
                                    }
                                } else {
                                    break;
                                }
                            }

                        } else {

                            for (int i = 1; i < winlen; i++) {
                                // vprintf("pkt# %ld -> status: %d\n", pkts[(sq+i)%winlen].seq_num, pkts[(sq+i)%winlen].status);
                                // lprintf(f, "pkt# %ld -> status: %d\n", pkts[(sq+i)%winlen].seq_num, pkts[(sq+i)%winlen].status);
                                if (pkts[(sq + i) % winlen].status == ACKED) {
                                    riseBase++;
                                    pkts[(sq + i) % winlen].status = NODATA;
                                    t_adpt_end[(sq + i) % winlen].tv_sec = 0;
                                    t_adpt_end[(sq + i) % winlen].tv_usec = 0;
                                    if ((long) pkts[(sq + i) % winlen].seq_num + 1 == num_packets) {
                                        vprintf("Tot Bytes readed: %ld\n", readed);
                                        vprintf("Tot Bytes sent: %ld\n", sent);
                                        tprintf(f, "Tot Bytes readed: %ld\n", readed);
                                        tprintf(f, "Tot Bytes sent: %ld\n", sent);
                                        ack_checker_quitted = 1;
                                        abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                                               "pthread_mutex_unlock() in ack_checker error");
                                        vprintf(GREEN "ack_checker quitting now...\n" DEFAULT);
                                        vprintf(GREEN "canceling sender thread now...\n" DEFAULT);
                                        abort_on_pthread_error(pthread_cancel(sender_thread),
                                                               "pthread_cancel() in ack_checker failed");
                                        pthread_exit(NULL);
                                    }
                                } else {
                                    break;
                                }
                            }
                        }

                        // lprintf(f, "riseBase: %d\n", riseBase);

                        // move window
                        base += riseBase;
                        pkts[sq % winlen].status = NODATA;
                        t_adpt_end[sq % winlen].tv_sec = 0;
                        t_adpt_end[sq % winlen].tv_usec = 0;

                        // // progress bar
                        // if (progress_bar_enabled) {
                        //     int barWidth = 50;

                        //     progress = base * 100 / num_packets;

                        //     // printf("progress: %f, step: %ld\n", progress, base);

                        //     printf(GREEN "[");
                        //     int pos = barWidth * progress / 100;

                        //     for (int i = 0; i <= barWidth; i++) {
                        //         if (i < pos)
                        //             printf("=");
                        //         else if (i == pos)
                        //             printf(">");
                        //         else
                        //             printf(" ");
                        //     }

                        //     printf("] %d%%\r" DEFAULT, (int) (progress));
                        //     fflush(stdout);
                        // }

                        abort_on_pthread_error(pthread_cond_signal(&cnd_not_full),
                                               "pthread_cond_signal() in ack_checker error");
                    }
                    // lprintf(f, "@END -> sq: %ld - base: %ld\n", sq, base);

                    if (for_test)
                        fflush(f);

                    // pkts[sq%winlen].status = NODATA;
                    // abort_on_pthread_error(pthread_cond_signal(&cnd_not_full), "pthread_cond_signal() in ack_checker error");
                }
            }
            // abort_on_pthread_error(pthread_cond_signal(&cnd_not_full), "pthread_cond_signal() in ack_checker error");
            abort_on_pthread_error(pthread_mutex_unlock(&mtx),
                                   "pthread_mutex_unlock() in ack_checker error");
        }
    }

    ack_checker_quitted = 1;
    vprintf(GREEN "ack_checker quitting now...\n" DEFAULT);
    vprintf(GREEN "canceling sender thread now...\n" DEFAULT);
    abort_on_pthread_error(pthread_cancel(sender_thread), "pthread_cancel() in ack_checker failed");
    pthread_exit(NULL);
}

int timeout_checker(int pos)
{
    abort_on_error(gettimeofday(&t_end[pos], NULL) == -1,
                   "Error in gettimeofday() in timeout_checker");

    if ((t_end[pos].tv_sec * 1000000 + t_end[pos].tv_usec) -
        (t_start[pos].tv_sec * 1000000 + t_start[pos].tv_usec) > timeout) {
        return 1;
    } else {
        return 0;
    }
}

long calc_adpt_timeout(int pos)
{
    long sample_RTT;
    long a1;
    long a2;
    long c1;
    long diff;
    long c2;

    tprintf(ftime, "%ld, %ld, ", t_start[pos].tv_sec * 10000000 + t_start[pos].tv_usec,
            t_adpt_end[pos].tv_sec * 10000000 + t_adpt_end[pos].tv_usec);
    // lprintf(ftime, "%lld"SPACE_LOG"%ld"SPACE_LOG"%ld"SPACE_LOG, measurement, t_start[pos].tv_sec*10000000+t_start[pos].tv_usec, t_adpt_end.tv_sec*10000000+t_adpt_end.tv_usec);

    sample_RTT =
        ((t_adpt_end[pos].tv_sec * 1000000) + t_adpt_end[pos].tv_usec) -
        ((t_start[pos].tv_sec * 1000000) + t_start[pos].tv_usec);
    tprintf(ftime, "%ld, ", sample_RTT);
    // lprintf(ftime, "%ld"SPACE_LOG, sample_RTT);

    a1 = (1 - ALPHA) * estimated_RTT;
    a2 = ALPHA * sample_RTT;
    estimated_RTT = a1 + a2;
    tprintf(ftime, "%ld, ", estimated_RTT);
    // lprintf(ftime, "%ld"SPACE_LOG, estimated_RTT);

    c1 = (1 - BETHA) * dev_RTT;
    diff = sample_RTT - estimated_RTT;
    if (diff < 0) {
        diff = diff * -1;
    }
    c2 = BETHA * diff;
    dev_RTT = c1 + c2;
    tprintf(ftime, "%ld, ", dev_RTT);
    // lprintf(ftime, "%ld"SPACE_LOG, dev_RTT);

    tprintf(ftime, "%ld\n", estimated_RTT + dev_RTT * 4);
    if (for_test && adaptive)
        fflush(ftime);

    measurement++;

    return estimated_RTT + dev_RTT * 4;
}
