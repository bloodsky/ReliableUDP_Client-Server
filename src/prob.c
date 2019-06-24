/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Probability Functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "macro.h"

int execute_with_prob(double);
int rand_seq_num(void);

int execute_with_prob(double prob)
{
    // srand((unsigned int) time(NULL));

    double random = (double) (rand() % 10) / 10;
    // printf("RANDOM_NUM: %f\n", random);

    if (random >= prob) {
        // printf("Sent\n");
        return 1;
    } else {
        // printf("Not sent\n");
        return 0;
    }
}

int rand_seq_num(void)
{
    // srand((unsigned int) time(NULL));

    int random = (int) (rand() % 1000);
    // printf("seq_num: %d\n", random);

    return random;
}

// int main(int argc, char const *argv[])
// {
//  srand((unsigned int) time(NULL));

//  for (int i = 0; i < 20; ++i) {
//  //  float random = (float) (rand() % 10) / 10;
//  //  printf("num: %f -> ", random);
//      execute_with_prob(PROB);
//  }

//  // for (int i = 0; i < 10; ++i) {

//  //  float random = (float) (rand() % 10) / 10;
//  //  printf("num: %f -> ", random);

//  //  if (random >= PROB) {
//  //      printf("Sent\n");
//  //      // return 1;
//  //  } else {
//  //      printf("Not sent\n");
//  //      // return 0;
//  //  }
//  // }

//  exit(EXIT_SUCCESS);
// }
