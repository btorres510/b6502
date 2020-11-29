#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "b6502/base.h"

static struct option long_options[]
    = {{"interactive", no_argument, 0, 'i'}, {"rom", no_argument, 0, 'r'}, {0, 0, 0, 0}};

int main(int argc, char **argv) {
  int c = 0;
  char *rom = NULL;
  bool interactive = false;

  opterr = 0;  // Disable getopt() error printing
  while ((c = getopt_long(argc, argv, "sr:", long_options, NULL)) != -1) {
    switch (c) {
      case 's':
        interactive = true;
        break;
      case 'r':
        rom = optarg;
        break;
      case '?':
        if (optopt == 'r' || strcmp(optarg, "rom") != 0) {
          LOG_ERROR("Option -%c requires an argument.\n", optopt);
        } else if (isprint(optopt)) {
          LOG_ERROR("Unknown option `-%c'.\n", optopt);
        } else {
          LOG_ERROR("Unknown option character `\\x%x'.\n", optopt);
        }

        return EXIT_FAILURE;
      default:
        abort();
    }
  }

  printf("Hello world!");
  return EXIT_SUCCESS;
}
