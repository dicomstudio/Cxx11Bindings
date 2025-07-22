#include "stream_implementation.hxx"

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>  // _chsize_s
#else
#include <unistd.h>  // ftruncate
#endif

// Truncate an open FILE* stream to a given size (in bytes).
// Returns 0 on success, -1 on error.
int truncate_file_fp(FILE* fp, int64_t new_size) {
  if (!fp || new_size < 0) {
    return -1;
  }

#ifdef _WIN32
  int fd = _fileno(fp);
  if (fd == -1) {
    return -1;
  }

  if (_chsize_s(fd, new_size) != 0) {
    return -1;
  }
#else
  int fd = fileno(fp);
  if (fd == -1) {
    return -1;
  }

  if (ftruncate(fd, new_size) != 0) {
    return -1;
  }
#endif

  return 0;
}
