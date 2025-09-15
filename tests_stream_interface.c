#include "stream_interface_impl.h"

#include <stdlib.h>
#include <stddef.h>

struct file_stream {
  struct c11_stream super;
  /* data */
  FILE* file;
};

CXX11_BINDINGS_EXPORT int test_c11_file_stream_create1(
  struct c11_stream** p_self, const char* filename, const char* mode);
CXX11_BINDINGS_EXPORT int test_c11_file_stream_create2(
  struct c11_stream** p_self, const wchar_t* filename, const wchar_t* mode);
CXX11_BINDINGS_EXPORT int test_c11_file_stream_destroy(struct c11_stream* self);

static buf_size my_read(struct c11_stream* self, byte* buffer,
                        const buf_size count) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const buf_size read = fread_file_fp(stream, buffer, count);
  return read;
}

static buf_size my_write(struct c11_stream* self, const byte* buffer,
                         const buf_size count) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const buf_size written = fwrite_file_fp(stream, buffer, count);
  return written;
}

static stream_offset my_seek(struct c11_stream* self,
                             const stream_offset offset, const seek_dir dir) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const int ret32 = fseek_file_fp(stream, offset, dir);
  if (ret32 < 0) return ret32;
  const int64_t ret64 = ftell_file_fp(stream);
  if (ret64 < 0) return ret64;
  return ret64;
}

static int my_flush(struct c11_stream* self) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const int ret = fflush_file_fp(stream);
  return ret;
}

static stream_length my_trunc(struct c11_stream* self,
                              const stream_length new_size) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const int64_t ret = ftruncate_file_fp(stream, new_size);
  return ret;
}

int test_c11_file_stream_create1(struct c11_stream** p_self,
                                 const char* filename, const char* mode) {
  struct file_stream* self = malloc(sizeof(*self));
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

int test_c11_file_stream_create2(struct c11_stream** p_self,
                                 const wchar_t* filename, const wchar_t* mode) {
  struct file_stream* self = malloc(sizeof(*self));
  if (self) {
    *p_self = &self->super;
    struct c11_stream* stream = &self->super;
    stream->read = my_read;
    stream->write = my_write;
    stream->seek = my_seek;
    stream->flush = my_flush;
    stream->trunc = my_trunc;
#ifdef _MSC_VER
    FILE* file = _wfopen(filename, mode);
#else
    FILE* file = NULL;
#endif
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

int test_c11_file_stream_destroy(struct c11_stream* self) {
  struct file_stream* stream_file = (struct file_stream*)self;
  FILE* file = stream_file->file;
  free(stream_file);
  const int ret = fclose(file);
  if (ret == 0) {
    return 0;
  }
  return -1;
}
