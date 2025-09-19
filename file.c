#include "errno.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Enum for file types
typedef enum FileTypes
{
  DATA,
  EMPTY,
  ASCII,
  ISO,
  UTF8
} FileTypes;

// Error printing function --> Prints error message if file cannot be read or opened
int print_error(char *path, int errnum)
{
  return fprintf(stdout, "%s: cannot determine (%s)\n",
                 path, strerror(errnum));
}

// Converts file type enum to human-readible string
const char *fileTypeToStr(FileTypes type)
{
  switch (type)
  {
  case EMPTY:
    return "Empty file";
  case ASCII:
    return "ASCII text";
  case ISO:
    return "ISO-8859 text";
  case UTF8:
    return "UTF-8 text";
  default:
    return "Data file";
  }
}

// ASCII checker (printable + allowed control chars)
bool is_ascii(unsigned int byte)
{
  if (byte >= 7 && byte <= 13)
    return true; // \a \b \t \n \v \f \r
  if (byte == 27)
    return true; // ESC
  if (byte >= 32 && byte <= 126)
    return true; // Printable ASCII
  return false;
}

// Checks if a single byte is valid ISO-8859 (Latin-1)
// ISO-8859 included:
// - All ASCII Bytes
// - Extended range (160-255)
bool is_iso8859(unsigned int byte)
{
  return (is_ascii(byte) || (byte >= 160 && byte <= 255));
}

// Checks if file is valid UTF-8
// Implements strict multi-byte validation using UTF-8 byte patterns
// 1-byte: 0xxxxxxx
// 2-byte: 110xxxxx 10xxxxxx
// 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
// 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
bool is_utf8(FILE *file)
{
  unsigned char c;

  while (fread(&c, 1, 1, file) == 1)
  {
    if (is_ascii(c))
    {
      continue; // ASCII bytes are always valid
    }
    else if ((c & 0xE0) == 0xC0)
    { // 2-byte sequence
      unsigned char c2;
      if (fread(&c2, 1, 1, file) != 1 || (c2 & 0xC0) != 0x80)
        return false;
    }
    else if ((c & 0xF0) == 0xE0)
    { // 3-byte sequence
      unsigned char c2, c3;
      if (fread(&c2, 1, 1, file) != 1 || (c2 & 0xC0) != 0x80 ||
          fread(&c3, 1, 1, file) != 1 || (c3 & 0xC0) != 0x80)
        return false;
    }
    else if ((c & 0xF8) == 0xF0)
    { // 4-byte sequence
      unsigned char c2, c3, c4;
      if (fread(&c2, 1, 1, file) != 1 || (c2 & 0xC0) != 0x80 ||
          fread(&c3, 1, 1, file) != 1 || (c3 & 0xC0) != 0x80 ||
          fread(&c4, 1, 1, file) != 1 || (c4 & 0xC0) != 0x80)
        return false;
    }
    else
    {
      return false; // Invalid UTF-8 leading byte
    }
  }

  fseek(file, 0, SEEK_SET); // Reset file pointer after UTF-8 reading.
  return true;              // All bytes are valid UTF-8 encoding.
}

// Filetype checker --> determines File type
FileTypes checkType(FILE *file)
{
  // Boolean flags
  bool only_ascii = true; // Tracks if all bytes are ASCII
  bool only_iso = true;   // Tracks if all bytes are ISO-8859
  bool empty = true;      // Tracks if file is empty.

  unsigned char c;

  while (fread(&c, 1, 1, file) == 1)
  {
    empty = false; // File is not empty
    if (!is_ascii(c))
      only_ascii = false;
    if (!is_iso8859(c))
      only_iso = false;

    // If file is not ASCII nor ISO-8859 --> Stop reading
    if (!only_ascii && !only_iso)
      break;
  }

  if (empty)
    return EMPTY; // Empty file
  if (only_ascii)
    return ASCII; // All bytes are ASCII
  if (only_iso)
    return ISO; // All bytes are ISO-8859

  // Reset file pointer for UTF-8 check
  fseek(file, 0, SEEK_SET);
  if (is_utf8(file))
    return UTF8;

  // Else treat as generic data
  return DATA;
}

//----------------
// MAIN PROGRAM
//----------------
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: file path\n");
    return EXIT_FAILURE;
  }

  // Opens file in binary mode
  FILE *file = fopen(argv[1], "rb");
  if (!file)
  {
    print_error(argv[1], errno);
    return EXIT_FAILURE;
  }

  // Determine file type and print
  FileTypes type = checkType(file);

  printf("%s: %s\n", argv[1], fileTypeToStr(type));

  fclose(file);

  return EXIT_SUCCESS;
}
