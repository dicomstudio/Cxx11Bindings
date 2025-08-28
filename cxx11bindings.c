#include "cxx11bindings.h"

CXX11_BINDINGS_EXPORT buf_size tc11_stream_read(struct c11_stream* c11_stream,
                                                byte* buffer, buf_size count);
CXX11_BINDINGS_EXPORT buf_size tc11_stream_write(struct c11_stream* c11_stream,
                                                 const byte* buffer,
                                                 buf_size count);
CXX11_BINDINGS_EXPORT stream_offset tc11_stream_seek(
    struct c11_stream* c11_stream, stream_offset off, seek_dir dir);
CXX11_BINDINGS_EXPORT int tc11_stream_flush(struct c11_stream* c11_stream);
CXX11_BINDINGS_EXPORT stream_length
tc11_stream_trunc(struct c11_stream* c11_stream, stream_length size);

buf_size tc11_stream_read(struct c11_stream* c11_stream, byte* buffer,
                          const buf_size count) {
  return c11_stream_read(c11_stream, buffer, count);
}

buf_size tc11_stream_write(struct c11_stream* c11_stream, const byte* buffer,
                           const buf_size count) {
  return c11_stream_write(c11_stream, buffer, count);
}

stream_offset tc11_stream_seek(struct c11_stream* c11_stream,
                               const stream_offset off, const seek_dir dir) {
  return c11_stream_seek(c11_stream, off, dir);
}

int tc11_stream_flush(struct c11_stream* c11_stream) {
  return c11_stream_flush(c11_stream);
}

stream_length tc11_stream_trunc(struct c11_stream* c11_stream,
                                const stream_length size) {
  return c11_stream_trunc(c11_stream, size);
}