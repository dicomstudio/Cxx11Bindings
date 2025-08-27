#include "cxx11bindings.h"

#ifdef _WIN32
#include <corerror.h>
#include <winerror.h>
#else
#define E_POINTER 0x80004003
#define COR_E_NOTSUPPORTED 0x80131515
#endif

buf_size c11_stream_read(struct c11_stream* c11_stream, byte* buffer,
                         const buf_size count) {
  if (!c11_stream) {
    return E_POINTER;
  }
  if (!c11_stream->read) {
    return COR_E_NOTSUPPORTED;
  }

  // else
  return c11_stream->read(buffer, count);
}

buf_size c11_stream_write(struct c11_stream* c11_stream, const byte* buffer,
                          const buf_size count) {
  if (!c11_stream) {
    return E_POINTER;
  }
  if (!c11_stream->write) {
    return COR_E_NOTSUPPORTED;
  }

  // write
  return c11_stream->write(buffer, count);
}

stream_offset c11_stream_seek(struct c11_stream* c11_stream,
                              const stream_offset off, const seek_dir dir) {
  if (!c11_stream) {
    return E_POINTER;
  }
  if (!c11_stream->seek) {
    return COR_E_NOTSUPPORTED;
  }

  // else
  return c11_stream->seek(off, dir);
}

int c11_stream_flush(struct c11_stream* c11_stream) {
  if (!c11_stream) {
    return E_POINTER;
  }
  if (!c11_stream->flush) {
    return COR_E_NOTSUPPORTED;
  }

  // else
  return c11_stream->flush();
}

stream_length c11_stream_trunc(struct c11_stream* c11_stream,
                               const stream_length size) {
  if (!c11_stream) {
    return E_POINTER;
  }
  if (!c11_stream->trunc) {
    return COR_E_NOTSUPPORTED;
  }

  // else
  return c11_stream->trunc(size);
}
