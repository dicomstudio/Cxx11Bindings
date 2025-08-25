#ifndef CXX11BINDINGS_H
#define CXX11BINDINGS_H

#include <stddef.h>  // size_t
#include <stdint.h>
#include <string.h>

#ifdef __GNUC__
#define CXX11_BINDINGS_EXPORT __attribute__((visibility("default")))
#define CXX11_CHECK_RETURN __attribute__((__warn_unused_result__))
#else
#define CXX11_BINDINGS_EXPORT __declspec(dllexport)

#define CXX11_CHECK_RETURN
#endif

#ifdef __cplusplus
extern "C" {

#endif
// C ABI to be used from C# P/Invoke or Python.ctypes
// struct allocation must be done in the target type and not native type to
// allow clear separation (no tight coupling)

// On Linux, write() (and similar system calls) will transfer at most
// 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes actually
// transferred.
typedef uint8_t byte;
// buffer size
typedef int32_t buf_size;
// stream offset (aka off_t)
typedef int64_t stream_offset;
// stream length (aka off_t), std::streamsize seems to be signed in c++
typedef int64_t stream_length;
// seek set/cur/end type:
typedef int seek_dir;

enum seek_dirs {
  seek_beg = 0,
  seek_cur = 1,
  seek_end = 2,
};

// 'c11' is the namespace, 'stream' is the type:
struct c11_stream;
// function declaration for stream interface
typedef buf_size (*read_fn)(byte* buffer, buf_size count);
typedef buf_size (*write_fn)(const byte* buffer, buf_size count);
typedef stream_offset (*seek_fn)(stream_offset off, seek_dir dir);
typedef int (*flush_fn)(void);
typedef stream_length (*trunc_fn)(stream_length size);

// https://stackoverflow.com/questions/29631692/self-referencing-class-concrete-python-class-from-c-interface
// Provides a simple basic stream interface, no buffering logic. Simply forward
// calls to actual implementation this is meant to provide a bridge from c# to
// c/c++
struct c11_stream {
  read_fn read;
  write_fn write;
  seek_fn seek;
  flush_fn flush;
  trunc_fn trunc;
};

/* c11 stream interface */
CXX11_BINDINGS_EXPORT buf_size c11_stream_read(struct c11_stream* c11_stream,
                                               byte* buffer, buf_size count);
CXX11_BINDINGS_EXPORT buf_size c11_stream_write(struct c11_stream* c11_stream,
                                                const byte* buffer,
                                                buf_size count);
CXX11_BINDINGS_EXPORT stream_offset
c11_stream_seek(struct c11_stream* c11_stream, stream_offset off, seek_dir dir);
CXX11_BINDINGS_EXPORT int c11_stream_flush(struct c11_stream* c11_stream);
CXX11_BINDINGS_EXPORT stream_length
c11_stream_trunc(struct c11_stream* c11_stream, stream_length size);

#if 0
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static inline buf_size c11_streambuf_read(struct c11_streambuf* c11_streambuf,
                                          byte* buffer, const buf_size count) {
  // user asks us to read 'count' bytes into 'buffer'
  struct managed_buffer* managed_buffer = &c11_streambuf->buffer;
  assert(managed_buffer->size != 0);
  // do it in chunk of buffer size:
  buf_size total_read = 0;
  while (total_read < count) {
    const buf_size to_read =
        MIN(count - total_read, (buf_size)(managed_buffer->size));
    assert(to_read > 0);
    const buf_size ret = managed_buffer->buf_read(to_read);
    // do not check ret == 0 here
    if (ret < 0) {
      return ret; // handle error
    }
    memcpy(buffer + total_read, managed_buffer->data, ret);
    total_read += ret;
    // add an extra 'if' to skip next potential expensive 'buf_read' callback
    if (ret < to_read) {
      return total_read; // end of stream
    }
  }
  return total_read;
}

static inline buf_size c11_streambuf_write(struct c11_streambuf* c11_streambuf,
                                           const byte* buffer,
                                           const buf_size count) {
  struct managed_buffer* managed_buffer = &c11_streambuf->buffer;
  // do it in chunk of buffer size:
  buf_size total_write = 0;
  while (total_write < count) {
    const buf_size to_write =
        MIN(count - total_write, (buf_size)(managed_buffer->size));
    assert(to_write > 0);
    memcpy(managed_buffer->data, buffer + total_write, to_write);
    const buf_size ret = managed_buffer->buf_write(to_write);
    assert(ret != 0);
    if (ret < 0) {
      return ret; // handle error
    }
    total_write += ret;
  }
  return total_write;
}
#undef MIN
#endif

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // CXX11BINDINGS_H
