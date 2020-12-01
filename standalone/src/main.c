#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "b6502/base.h"

static struct option long_options[] = {{"rom", required_argument, 0, 'r'},
                                       {"system", required_argument, 0, 's'},
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

static void print_help(void) {}

int main(int argc, char **argv) {
  int c = 0;
  char *rom = NULL;
  char *sys = NULL;

  while ((c = getopt_long(argc, argv, "r:s:h", long_options, NULL)) != -1) {
    switch (c) {
      case 's':
        sys = optarg;
        break;
      case 'r':
        rom = optarg;
        break;
      case 'h':
        print_help();
        break;
      case '?':
        return EXIT_FAILURE;
      default:
        abort();
    }
  }

  return EXIT_SUCCESS;
}
