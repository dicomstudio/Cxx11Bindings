#include "cxx11bindings.h"

#define CHECK_ARGS(s, e1, f, e2) \
  do {                           \
    if (!(s)) {                  \
      return (e1);               \
    }                            \
    if (!((s)->f)) {             \
      return (e2);               \
    }                            \
  } while (0)

buf_size c11_stream_read(struct c11_stream* c11_stream, byte* buffer,
                         const buf_size count) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, read, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->read(buffer, count);
}

buf_size c11_stream_write(struct c11_stream* c11_stream, const byte* buffer,
                          const buf_size count) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, write, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->write(buffer, count);
}

stream_offset c11_stream_seek(struct c11_stream* c11_stream,
                              const stream_offset off, const seek_dir dir) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, seek, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->seek(off, dir);
}

int c11_stream_flush(struct c11_stream* c11_stream) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, flush, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->flush();
}

stream_length c11_stream_trunc(struct c11_stream* c11_stream,
                               const stream_length size) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, trunc, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->trunc(size);
}