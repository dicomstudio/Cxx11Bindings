#include "cxx11bindings.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>  // _chsize_s
#else
#include <sys/types.h>
#include <unistd.h>  // ftruncate
#endif

// Truncate an open FILE* stream to a given size (in bytes).
// Returns new size on success, -1 on error.
static inline int64_t truncate_file_fp(FILE* fp, int64_t new_size) {
  if (!fp || new_size < 0) {
    return -1;
  }

#ifdef _WIN32
  const int fd = _fileno(fp);
  if (fd == -1) {
    return -1;
  }

  if (_chsize_s(fd, new_size) != 0) {
    return -1;
  }
#else
  const int fd = fileno(fp);
  if (fd == -1) {
    return -1;
  }

  if (ftruncate(fd, new_size) != 0) {
    return -1;
  }
#endif

  return new_size;
}

// Mostly to mimic an external stream source
struct stream_file {
  struct c11_stream super;
  /* data */
  FILE* file;
};

struct stream_file* global;

static buf_size my_read(byte* buffer, const buf_size count) {
  assert(count>0);
  FILE* stream = global->file;
  const size_t read = fread(buffer, 1, count, stream);
  if (read == (size_t)count) {
    return (buf_size)read;
  }
  return -1;
}

static buf_size my_write(const byte* buffer, const buf_size count) {
  assert(count>0);
  FILE* stream = global->file;
  const size_t written = fwrite(buffer, 1, count, stream);
  if (written == (size_t)count) {
    return (buf_size)written;
  }
  return -1;
}

static inline int fseek_file_fp(FILE* stream, int64_t offset, int whence) {
#ifdef _WIN32
  return _fseeki64(stream, offset, whence);
#else
  return fseeko(stream, offset, whence);
#endif
}

static inline int64_t ftell_file_fp(FILE* stream) {
#ifdef _WIN32
  return _ftelli64(stream);
#else
  return ftello(stream);
#endif
}

static stream_offset my_seek(const stream_offset offset, const int seek_dir) {
  FILE* stream = global->file;
  int whence;
  switch (seek_dir) {
    case seek_beg:
      whence = SEEK_SET;
      break;
    case seek_cur:
      whence = SEEK_CUR;
      break;
    case seek_end:
      whence = SEEK_END;
      break;
    default:
      return -1;
  }

  const int ret = fseek_file_fp(stream, offset, whence);
  if (ret == 0) {
    const int64_t pos = ftell_file_fp(stream);
    return pos;
  }
  return -1;
}

static int my_flush(void) {
  FILE* stream = global->file;
  if (stream) {
    const int ret = fflush(stream);
    if (ret == EOF) {
      // FIXME: errno
      return -1;
    }
    return 0;
  }
  return -1;
}

static stream_length my_trunc(const stream_length new_size) {
  FILE* stream = global->file;
  const int64_t ret = truncate_file_fp(stream, new_size);
  return ret;
}

int c11_stream_file_create(struct c11_stream** p_self, const char* filename,
                           const char* mode) {
  struct stream_file* self = malloc(sizeof(*self));
  if (self) {
    *p_self = &self->super;
    struct c11_stream* stream = &self->super;
    stream->read = my_read;
    stream->write = my_write;
    stream->seek = my_seek;
    stream->flush = my_flush;
    stream->trunc = my_trunc;
    FILE* file = fopen(filename, mode);
    if (file) {
      self->file = file;
      return 0;
    }
    // else error:
    free(self);
  }
  *p_self = NULL;
  return -1;
}

int c11_stream_file_destroy(struct c11_stream* self) {
  struct stream_file* stream_file = (struct stream_file*)self;
  FILE* file = stream_file->file;
  free(stream_file);
  const int ret = fclose(file);
  if (ret == 0) {
    return 0;
  }
  return -1;
}

int main(const int argc, char* argv[]) {
  if (argc < 2) {
    return 1;
  }
  const char* filename = argv[1];
  struct c11_stream* stream;
  int ret = c11_stream_file_create(&stream, filename, "wb");
  global = (struct stream_file*)stream;
  if (ret < 0) return 1;
  const char data[] = "Hello, World!";
  const buf_size size = c11_stream_write(stream, data, sizeof data);
  if (size < 0) return 1;
  const stream_length l = c11_stream_trunc(stream, 1024);
  if (l < 0) return 1;
  ret = c11_stream_file_destroy(stream);
  if (ret < 0) return 1;
  return 0;
}
