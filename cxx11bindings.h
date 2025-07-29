#pragma once
#include <stddef.h>  // size_t
#include <stdint.h>
#ifdef __GNUC__
#define CXX11_BINDINGS_EXPORT __attribute__((visibility("default")))
#else
#define CXX11_BINDINGS_EXPORT __declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C" {
#endif
// C ABI to be used from C# P/Invoke or Python.ctypes
// struct allocation must be done in the target type and not native type to
// allow clear separation (no tight coupling)

// On  Linux,  write()  (and similar system calls) will transfer at most
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

// callbacks for stream interface
typedef buf_size (*read_fn)(byte* buffer, buf_size count);
typedef buf_size (*write_fn)(const byte* buffer, buf_size count);
typedef stream_offset (*seek_fn)(stream_offset off, seek_dir dir);
typedef int (*flush_fn)();
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

typedef buf_size (*buf_read_fn)(buf_size count);
typedef buf_size (*buf_write_fn)(buf_size count);

struct managed_buffer {
  buf_read_fn buf_read;
  buf_write_fn buf_write;
  byte* data;
  // use `size_t` to have 8 bytes on x64, for alignment. But technically only
  // hold int32
  size_t size;
};

// provide a slightly more complex interface akin to FILE* or std::streambuf
struct c11_streambuf {
  struct managed_buffer buffer;
  seek_fn seek;
  flush_fn flush;
  trunc_fn trunc;
};

CXX11_BINDINGS_EXPORT buf_size c11_streambuf_read(
    struct c11_streambuf* c11_streambuf, byte* buffer, buf_size count);
CXX11_BINDINGS_EXPORT buf_size c11_streambuf_write(
    struct c11_streambuf* c11_streambuf, const byte* buffer, buf_size count);
CXX11_BINDINGS_EXPORT stream_offset c11_streambuf_seek(
    struct c11_streambuf* c11_streambuf, stream_offset off, seek_dir dir);
CXX11_BINDINGS_EXPORT int c11_streambuf_flush(
    struct c11_streambuf* c11_streambuf);
CXX11_BINDINGS_EXPORT stream_length
c11_streambuf_trunc(struct c11_streambuf* c11_streambuf, stream_length size);

#ifdef __cplusplus
}  // end extern "C"
#endif
