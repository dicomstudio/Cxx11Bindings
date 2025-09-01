#include "cxx11bindings.h"
#include "stream_interface_impl.h"

#include <stdio.h>
#include <stdlib.h>

// Mostly to mimic an external stream source
struct stream_file {
  struct c11_stream super;
  /* data */
  FILE* file;
};

struct stream_file* global;

static buf_size my_read(byte* buffer, const buf_size count) {
  FILE* stream = global->file;
  const buf_size read = fread_file_fp(stream, buffer, count);
  return read;
}

static buf_size my_write(const byte* buffer, const buf_size count) {
  FILE* stream = global->file;
  const buf_size written = fwrite_file_fp(stream, buffer, count);
  return written;
}

static stream_offset my_seek(const stream_offset offset, const seek_dir dir) {
  FILE* stream = global->file;
  const int ret32 = fseek_file_fp(stream, offset, dir);
  if (ret32 < 0) return ret32;
  const int64_t ret64 = ftell_file_fp(stream);
  if (ret64 < 0) return ret64;
  return ret64;
}

static int my_flush(void) {
  FILE* stream = global->file;
  const int ret = fflush_file_fp(stream);
  return ret;
}

static stream_length my_trunc(const stream_length new_size) {
  FILE* stream = global->file;
  const int64_t ret = ftruncate_file_fp(stream, new_size);
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
