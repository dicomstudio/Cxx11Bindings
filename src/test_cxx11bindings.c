// cxx11bindings is a header-only library
// Mostly used to expose symbols for testing purpose.
#include <cxx11bindings/stream.h>

CXX11_BINDINGS_EXPORT buf_size test_c11_stream_read(
    struct c11_stream* c11_stream, byte* buffer, buf_size count);
CXX11_BINDINGS_EXPORT buf_size test_c11_stream_write(
    struct c11_stream* c11_stream, const byte* buffer, buf_size count);
CXX11_BINDINGS_EXPORT stream_offset test_c11_stream_seek(
    struct c11_stream* c11_stream, stream_offset off, seek_dir dir);
CXX11_BINDINGS_EXPORT int test_c11_stream_flush(struct c11_stream* c11_stream);
CXX11_BINDINGS_EXPORT stream_length
test_c11_stream_trunc(struct c11_stream* c11_stream, stream_length size);

buf_size test_c11_stream_read(struct c11_stream* c11_stream, byte* buffer,
                              const buf_size count) {
  return c11_stream_read(c11_stream, buffer, count);
}

buf_size test_c11_stream_write(struct c11_stream* c11_stream,
                               const byte* buffer, const buf_size count) {
  return c11_stream_write(c11_stream, buffer, count);
}

stream_offset test_c11_stream_seek(struct c11_stream* c11_stream,
                                   const stream_offset off,
                                   const seek_dir dir) {
  return c11_stream_seek(c11_stream, off, dir);
}

int test_c11_stream_flush(struct c11_stream* c11_stream) {
  return c11_stream_flush(c11_stream);
}

stream_length test_c11_stream_trunc(struct c11_stream* c11_stream,
                                    const stream_length size) {
  return c11_stream_trunc(c11_stream, size);
}
