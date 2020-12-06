#include "b6502/base.h"

#include <stdio.h>
#include <string.h>

int read_rom(const char *path, void *restrict dest, size_t size, size_t nmemb) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    LOG_ERROR("Error opening file: %s\n", strerror(errno));
    return -1;
  }

  if (fread(dest, size, nmemb, f) != nmemb) {
    LOG_ERROR("Unable to read in %zu bytes!\n", nmemb);
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
}

void dummy(void *UNUSED(obj)) {}
