// cxx11bindings is a header-only library
// Mostly used to expose symbols for testing purpose.
#include <cxx11bindings/stream.h>

#ifdef CXX11_BINDINGS_TEST_MODULE
#define CXX11_TEST_EXPORT CXX11_BINDINGS_EXPORT
#else
#define CXX11_TEST_EXPORT
#endif

CXX11_TEST_EXPORT buf_size test_c11_stream_read(
    struct c11_stream* c11_stream, byte* buffer, buf_size count);
CXX11_TEST_EXPORT buf_size test_c11_stream_write(
    struct c11_stream* c11_stream, const byte* buffer, buf_size count);
CXX11_TEST_EXPORT stream_offset test_c11_stream_seek(
    struct c11_stream* c11_stream, stream_offset off, seek_dir dir);
CXX11_TEST_EXPORT int test_c11_stream_flush(struct c11_stream* c11_stream);
CXX11_TEST_EXPORT stream_length
test_c11_stream_trunc(struct c11_stream* c11_stream, stream_length size);
CXX11_TEST_EXPORT int test_c11_stream_seek_direction(void);

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

struct seek_probe {
  struct c11_stream stream;
  stream_offset offset;
  seek_dir dir;
  int called;
};

static stream_offset record_seek(struct c11_stream* stream,
                                 const stream_offset offset,
                                 const seek_dir dir) {
  struct seek_probe* probe = (struct seek_probe*)stream;
  probe->offset = offset;
  probe->dir = dir;
  probe->called = 1;
  return offset;
}

int test_c11_stream_seek_direction(void) {
  static const seek_dir directions[] = {seek_beg, seek_cur, seek_end};
  struct c11_stream missing_callback = {0};
  size_t i;

  if (test_c11_stream_seek(NULL, 17, seek_beg) != C11_E_POINTER) {
    return C11_E_INVALIDARG;
  }
  if (test_c11_stream_seek(&missing_callback, 17, seek_beg) !=
      C11_E_NOTSUPPORTED) {
    return C11_E_INVALIDARG;
  }

  for (i = 0; i < sizeof directions / sizeof directions[0]; ++i) {
    struct seek_probe probe = {0};
    probe.stream.seek = record_seek;

    if (test_c11_stream_seek(&probe.stream, 17, directions[i]) != 17 ||
        !probe.called || probe.offset != 17 || probe.dir != directions[i]) {
      return C11_E_INVALIDARG;
    }
  }

  {
    struct seek_probe probe = {0};
    probe.stream.seek = record_seek;
    if (test_c11_stream_seek(&probe.stream, 17, (seek_dir)-1) !=
            C11_E_INVALIDARG ||
        probe.called) {
      return C11_E_INVALIDARG;
    }
  }
  return 0;
}
