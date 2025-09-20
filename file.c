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
    return "empty";
  case ASCII:
    return "ASCII text";
  case ISO:
    return "ISO-8859 text";
  case UTF8:
    return "UTF-8 text";
  default:
    return "data";
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
  unsigned char buffer[32];
  size_t bytes_read;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
    for (size_t i = 0; i < bytes_read; i++)
    {
      unsigned char c = buffer[i];

      if (is_ascii(c))
      {
        continue; // ASCII bytes are always valid
      }
      else if ((c & 0xE0) == 0xC0) // 2-byte sequence
      {
        if (++i >= bytes_read && fread(&c, 1, 1, file) != 1)
          return false;
        if ((buffer[i] & 0xC0) != 0x80)
          return false;
      }
      else if ((c & 0xF0) == 0xE0) // 3-byte sequence
      {
        for (int j = 0; j < 2; j++)
        {
          if (++i >= bytes_read && fread(&c, 1, 1, file) != 1)
            return false;
          if ((buffer[i] & 0xC0) != 0x80)
            return false;
        }
      }
      else if ((c & 0xF8) == 0xF0) // 4-byte sequence
      {
        for (int j = 0; j < 3; j++)
        {
          if (++i >= bytes_read && fread(&c, 1, 1, file) != 1)
            return false;
          if ((buffer[i] & 0xC0) != 0x80)
            return false;
        }
      }
      else
      {
        return false; // Invalid UTF-8 leading byte
      }
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
  bool stop_loop = false; // Tracks the loop.

  unsigned char buffer[32];
  size_t bytes_read;

  // Reading file in 32-byte chunks
  while (!stop_loop && (bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
    empty = false;
    for (size_t i = 0; i < bytes_read; i++)
    {
      if (!is_ascii(buffer[i]))
        only_ascii = false;
      if (!is_iso8859(buffer[i]))
        only_iso = false;

      // If file is not ASCII nor ISO-8859 -> loop terminates
      if (!only_ascii && !only_iso)
      {
        stop_loop = true;
        break; // exit inner loop
      }
    }
  }

  // Classsifying file type
  if (empty)
    return EMPTY;
  if (only_ascii)
    return ASCII;
  if (only_iso)
    return ISO;

  // Reset file pointer for UTF-8 checker
  fseek(file, 0, SEEK_SET);
  if (is_utf8(file))
    return UTF8;

  // Returns DATA
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
