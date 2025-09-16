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
    fprintf(stderr, "Usage: file path\n");
    return EXIT_FAILURE;
  }

  FILE *file = fopen(argv[1], "r");

  if (file == NULL || ferror(file)) {
    printf("%s: cannot determine (%s)\n", argv[1], strerror(errno));
    return EXIT_SUCCESS;
  }

  char buffer[32];
  size_t bytes_read = fread(buffer, 1, 32, file);

  filetype = ASCII | LATIN1 | UTF8 | DATA;

  if (bytes_read == 0) {
    filetype = EMPTY;
  }

  while (bytes_read > 0) {
    bytes_read = fread(buffer, 1, 32, file);

    if ((filetype & UTF8) && !is_utf8(buffer))
      filetype ^= UTF8;
    if ((filetype & LATIN1) && !is_latin1(buffer))
      filetype ^= LATIN1;
    if ((filetype & ASCII) && !is_ascii(buffer))
      filetype ^= ASCII;

    if (filetype == DATA)
      break;
  }

  fclose(file);

  if (filetype == EMPTY) {
    printf("%s: Empty\n", argv[1]);
    return EXIT_SUCCESS;
  }

  if (filetype == DATA) {
    printf("%s: Data\n", argv[1]);
    return EXIT_SUCCESS;
  }

  printf("%s: ", argv[1]);
  if (filetype & ASCII)
    printf("ASCII ");
  else if (filetype & LATIN1)
    printf("LATIN1 ");
  else if (filetype & UTF8)
    printf("UTF-8 ");

  printf("text\n");
  return EXIT_SUCCESS;
}

int is_ascii(char buffer[32]) {
  for (int i = 0; i < 32; i++) {
    char b = buffer[i];

    if (b == 0 || b == 0x1A)
      continue; // null terminator and newline

    if ((b < 0x07 || b > 0x0D) && (b < 0x20 || b > 0x7E) && b != 0x1B) 
      return 0;
  }
  return 1;
}

int is_latin1(char buffer[32]) {
  if (is_ascii(buffer))
    return 1;

  for (int i = 0; i < 32; i++) {
    char b = buffer[i];

    if (b == 0 || b == 0x1A)
      continue; // null terminator and newline

    if (b > 128 && b < 160)
      return 0;
  }
  return 1;
}

int is_utf8(char buffer[32]) {
  for (int i = 0; i < 32; i++) {
    char b = buffer[i];

    if (b == 0 || b == 0x1A)
      continue; // null terminator and newline

    if (
      (b & 0b00000000)
      || (b & 0b11000000 && buffer[i+1] & 0b10000000)
      || (b & 0b11100000 && buffer[i+1] & 0b10000000 && buffer[i+2] & 0b10000000)
      || (b & 0b11110000 && buffer[i+1] & 0b10000000 && buffer[i+2] & 0b10000000 && buffer[i+3] & 0b10000000)
    )
      return 1;
  }
  return 0;
}
