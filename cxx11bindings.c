#include "cxx11bindings.h"

buf_size c11_stream_read(struct c11_stream* c11_stream, byte* buffer,
                         const buf_size count) {
  if (c11_stream && c11_stream->read) {
    return c11_stream->read(buffer, count);
  }
  return -1;
}

buf_size c11_stream_write(struct c11_stream* c11_stream, const byte* buffer,
                          const buf_size count) {
  if (c11_stream && c11_stream->write) {
    return c11_stream->write(buffer, count);
  }
  return -1;
}

stream_offset c11_stream_seek(struct c11_stream* c11_stream,
                              const stream_offset off, const seek_dir dir) {
  if (c11_stream && c11_stream->seek) {
    return c11_stream->seek(off, dir);
  }
  return -1;
}

int c11_stream_flush(struct c11_stream* c11_stream) {
  if (c11_stream && c11_stream->flush) {
    return c11_stream->flush();
  }
  return -1;
}

stream_length c11_stream_trunc(struct c11_stream* c11_stream,
                               const stream_length size) {
  if (c11_stream && c11_stream->trunc) {
    return c11_stream->trunc(size);
  }
  return -1;
}