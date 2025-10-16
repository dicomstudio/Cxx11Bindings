#include <cxx11bindings/file.h>

#include "stream_interface_impl.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct file_stream {
  struct c11_stream super;
  /* data */
  FILE* file;
  bool is_created;
};

static buf_size file_read(struct c11_stream* self, byte* buffer,
                          const buf_size count) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const buf_size read = fread_file_fp(stream, buffer, count);
  return read;
}

static buf_size file_write(struct c11_stream* self, const byte* buffer,
                           const buf_size count) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const buf_size written = fwrite_file_fp(stream, buffer, count);
  return written;
}

static stream_offset file_seek(struct c11_stream* self,
                               const stream_offset offset, const seek_dir dir) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const int ret32 = fseek_file_fp(stream, offset, dir);
  if (ret32 < 0) {
    return ret32;
  }
  const int64_t ret64 = ftell_file_fp(stream);
  if (ret64 < 0) return ret64;
  return ret64;
}

static int file_flush(struct c11_stream* self) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const int ret = fflush_file_fp(stream);
  return ret;
}

static stream_length file_trunc(struct c11_stream* self,
                                const stream_length new_size) {
  struct file_stream* fs = (struct file_stream*)self;
  FILE* stream = fs->file;
  const int64_t ret = ftruncate_file_fp(stream, new_size);
  return ret;
}

static int file_stream_init(struct c11_stream** p_self, FILE* file,
                            const bool created) {
  assert(file != NULL);
  struct file_stream* self = malloc(sizeof(*self));
  if (self) {
    *p_self = &self->super;
    struct c11_stream* stream = &self->super;
    stream->read = file_read;
    stream->write = file_write;
    stream->seek = file_seek;
    stream->flush = file_flush;
    stream->trunc = file_trunc;
    self->file = file;
    self->is_created = created;
    // success
    return 0;
  }
  *p_self = NULL;
  return C11_E_POINTER;
}

int c11_file_stream_open1(struct c11_stream** p_self, const char* filename,
                          const char* mode) {
  if (filename && mode) {
    FILE* file = fopen(filename, mode);
    if (file) {
      const int ret = file_stream_init(p_self, file, true);
      if (ret == 0) {
        return 0;
      }
      (void)fclose(file);
    }
    // else
    return C11_E_IO;
  }
  // else
  return C11_E_INVALIDARG;
}

int c11_file_stream_open2(struct c11_stream** p_self, const wchar_t* filename,
                          const wchar_t* mode) {
  if (filename && mode) {
#ifdef _MSC_VER
    FILE* file = _wfopen(filename, mode);
#else
    FILE* file = NULL;
#endif
    if (file) {
      const int ret = file_stream_init(p_self, file, true);
      if (ret == 0) {
        return 0;
      }
      (void)fclose(file);
    }
    // else
    return C11_E_IO;
  }
  // else
  return C11_E_INVALIDARG;
}

int c11_file_stream_destroy(struct c11_stream* self) {
  if (self) {
    struct file_stream* stream_file = (struct file_stream*)self;
    int ret = 0;
    if (stream_file->is_created) {
      FILE* file = stream_file->file;
      // fclose() returns EOF on error, makes it E_IO here:
      ret = fclose(file) == 0 ? 0 : C11_E_IO;
    }
    free(stream_file);
    return ret;
  }
  // else
  return C11_E_INVALIDARG;
}

int c11_file_stream_create(struct c11_stream** p_self, FILE* stream) {
  if (stream) {
    return file_stream_init(p_self, stream, false);
  }
  return C11_E_INVALIDARG;
}

#ifdef _WIN32
struct handle_stream {
  struct c11_stream super;
  /* data */
  HANDLE handle;
  bool is_created;
};

static buf_size handle_read(struct c11_stream* self, byte* buffer,
                            const buf_size count) {
  struct handle_stream* fs = (struct handle_stream*)self;
  const HANDLE handle = fs->handle;
  const buf_size read = read_handle(handle, buffer, count);
  return read;
}

static buf_size handle_write(struct c11_stream* self, const byte* buffer,
                             const buf_size count) {
  struct handle_stream* fs = (struct handle_stream*)self;
  const HANDLE handle = fs->handle;
  const buf_size written = write_handle(handle, buffer, count);
  return written;
}

static stream_offset handle_seek(struct c11_stream* self,
                                 const stream_offset offset,
                                 const seek_dir dir) {
  struct handle_stream* fs = (struct handle_stream*)self;
  const HANDLE handle = fs->handle;
  const int64_t pos = seek_handle(handle, offset, dir);
  return pos;
}

static int handle_flush(struct c11_stream* self) {
  struct handle_stream* fs = (struct handle_stream*)self;
  const HANDLE handle = fs->handle;
  const int ret = flush_handle(handle);
  return ret;
}

static stream_length handle_trunc(struct c11_stream* self,
                                  const stream_length new_size) {
  struct handle_stream* fs = (struct handle_stream*)self;
  const HANDLE handle = fs->handle;
  const int64_t ret = truncate_handle(handle, new_size);
  return ret;
}

static int handle_stream_init(struct c11_stream** p_self, const HANDLE handle,
                              const bool created) {
  assert(handle != NULL);
  struct handle_stream* self = malloc(sizeof(*self));
  if (self) {
    *p_self = &self->super;
    struct c11_stream* stream = &self->super;
    stream->read = handle_read;
    stream->write = handle_write;
    stream->seek = handle_seek;
    stream->flush = handle_flush;
    stream->trunc = handle_trunc;
    self->handle = handle;
    self->is_created = created;
    // success
    return 0;
  }
  *p_self = NULL;
  return C11_E_POINTER;
}

int c11_handle_stream_init(struct c11_stream** p_self, const HANDLE handle) {
  if (handle != NULL) return handle_stream_init(p_self, handle, false);
  return C11_E_INVALIDARG;
}
#else
struct fd_stream {
  struct c11_stream super;
  /* data */
  int fd;
  bool is_created;
};
static buf_size fd_read(struct c11_stream* self, byte* buffer,
                        const buf_size count) {
  struct fd_stream* fs = (struct fd_stream*)self;
  const int fd = fs->fd;
  const buf_size read = read_fd(fd, buffer, count);
  return read;
}
static buf_size fd_write(struct c11_stream* self, const byte* buffer,
                         const buf_size count) {
  struct fd_stream* fs = (struct fd_stream*)self;
  const int fd = fs->fd;
  const buf_size written = write_fd(fd, buffer, count);
  return written;
}
static stream_offset fd_seek(struct c11_stream* self,
                             const stream_offset offset, const seek_dir dir) {
  struct fd_stream* fs = (struct fd_stream*)self;
  const int fd = fs->fd;
  const int64_t pos = lseek_fd(fd, offset, dir);
  return pos;
}
static int fd_flush(struct c11_stream* self) {
  struct fd_stream* fs = (struct fd_stream*)self;
  const int fd = fs->fd;
  const int ret = fsync_fd(fd);
  return ret;
}

static stream_length fd_trunc(struct c11_stream* self,
                              const stream_length new_size) {
  struct fd_stream* fs = (struct fd_stream*)self;
  const int fd = fs->fd;
  const int64_t ret = ftruncate_fd(fd, new_size);
  return ret;
}

static int fd_stream_init(struct c11_stream** p_self, const int fd,
                          const bool created) {
  assert(fd >= 0);
  struct fd_stream* self = malloc(sizeof(*self));
  if (self) {
    *p_self = &self->super;
    struct c11_stream* stream = &self->super;
    stream->read = fd_read;
    stream->write = fd_write;
    stream->seek = fd_seek;
    stream->flush = fd_flush;
    stream->trunc = fd_trunc;
    self->fd = fd;
    self->is_created = created;
    // success
    return 0;
  }
  *p_self = NULL;
  return C11_E_POINTER;
}

int c11_fd_stream_init(struct c11_stream** p_self, int fd) {
  if (fd >= 0) {
    return fd_stream_init(p_self, fd, false);
  }
  return C11_E_INVALIDARG;
}
#endif
