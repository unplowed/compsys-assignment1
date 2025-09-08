#include "errno.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

enum Filetype {
  ASCII = 1 << 0,
  LATIN1 = 1 << 1,
  UTF8 = 1 << 2,
  DATA = 1 << 3,
  EMPTY = 1 << 4
} filetype;

int is_ascii(char buffer[32]);
int is_latin1(char buffer[32]);
int is_utf8(char buffer[32]);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: file <file>\n");
    return EXIT_FAILURE;
  }

  FILE *file = fopen(argv[1], "r");

  if (file == NULL) {
    printf("Couldn't open file '%s': %s\n", argv[1], strerror(errno));
    return EXIT_FAILURE;
  }
  if (ferror(file)) {
    printf("Couldn't read file '%s': %s\n", argv[1], strerror(errno));
    return EXIT_FAILURE;
  }

  char buffer[32];
  size_t bytes_read = fread(buffer, 1, 32, file);

  filetype = ASCII | LATIN1 | UTF8 | DATA;

  if (bytes_read == 0) {
    filetype = EMPTY;
  }

  while (bytes_read > 0) {
    bytes_read = fread(buffer, 1, 32, file);

    if (filetype & ASCII && !is_ascii(buffer))
      filetype = filetype ^ ASCII;

    if (filetype & LATIN1 && !is_latin1(buffer))
      filetype = filetype ^ LATIN1;

    // if (filetype & UTF8 && !is_utf8(buffer))
    //   filetype = filetype ^ UTF8;
  }

  if (filetype == EMPTY) {
    printf("%s: Empty", argv[1]);
    return EXIT_SUCCESS;
  }

  if (filetype == DATA) {
    printf("%s: Data", argv[1]);
    return EXIT_SUCCESS;
  }

  printf("%s: ", argv[1]);
  if (filetype & ASCII)
    printf("ASCII ");

  if (filetype & LATIN1)
    printf("LATIN1 ");

  if (filetype & UTF8)
    printf("UTF-8 ");

  printf("text\n");
  return EXIT_SUCCESS;
}

int is_ascii(char buffer[32]) {
  for (int i = 0; i < 32; i++) {
    char b = buffer[i];
    if ((b < 0x07 || b > 0x0D) && (b < 0x20 || b > 0x7E) && b != 0x1B)
      return 0;
  }
  return 1;
}

int is_latin1(char buffer[32]) {
  if (!is_ascii(buffer))
    return 1;

  for (int i = 0; i < 32; i++) {
    char b = buffer[i];
    if (b < 160)
      return 0;
  }
  return 1;
}

int if_utf8(char buffer[32]) {
  return 0;
}
