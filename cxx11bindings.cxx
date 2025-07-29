#include "cxx11bindings.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <new>

extern "C" {
buf_size c11_streambuf_read(c11_streambuf* c11_streambuf, byte* buffer,
                            const buf_size count) {
  // user asks us to read 'count' bytes into 'buffer'
  managed_buffer* managed_buffer = &c11_streambuf->buffer;
  assert(managed_buffer->size != 0);
  // do it in chunk of buffer size:
  buf_size total_read = 0;
  while (total_read < count) {
    const buf_size to_read = std::min(
        count - total_read, static_cast<buf_size>(managed_buffer->size));
    assert(to_read > 0);
    const int ret = managed_buffer->buf_read(to_read);
    // do not check ret == 0 here
    if (ret < 0) {
      return ret;  // handle error
    }
    std::memcpy(buffer + total_read, managed_buffer->data, ret);
    total_read += ret;
    // add an extra 'if' to skip next potential expensive 'read_into' callback
    if (ret < to_read) {
      return total_read;  // end of stream
    }
  }
  return total_read;
}

buf_size c11_streambuf_write(c11_streambuf* c11_streambuf, const byte* buffer,
                             const buf_size count) {
  managed_buffer* managed_buffer = &c11_streambuf->buffer;
  // do it in chunk of buffer size:
  buf_size total_write = 0;
  while (total_write < count) {
    const buf_size to_write = std::min(
        count - total_write, static_cast<buf_size>(managed_buffer->size));
    assert(to_write > 0);
    std::memcpy(managed_buffer->data, buffer + total_write, to_write);
    const int ret = managed_buffer->buf_write(to_write);
    assert(ret != 0);
    if (ret < 0) {
      return ret;  // handle error
    }
    total_write += ret;
  }
  return total_write;
}

stream_offset c11_streambuf_seek(c11_streambuf* c11_streambuf,
                                 const stream_offset off, const seek_dir dir) {
  const stream_offset ret = c11_streambuf->seek(off, dir);
  return ret;
}

int c11_streambuf_flush(c11_streambuf* c11_streambuf) {
  return c11_streambuf->flush();
}

stream_length c11_streambuf_trunc(c11_streambuf* c11_streambuf,
                                  const stream_length size) {
  return c11_streambuf->trunc(size);
}
}  // extern "C"
