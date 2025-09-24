#ifndef CXX11_BINDINGS_STREAM_H
#define CXX11_BINDINGS_STREAM_H

#include <stddef.h>  // size_t
#include <stdint.h>  // require c99

#define CXX11BINDINGS_VERSION_MAJOR 3
#define CXX11BINDINGS_VERSION_MINOR 1
#define CXX11BINDINGS_VERSION_PATCH 0

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

// Define our internal error codes to be used as return values
// they must be decoded at library level to be remapped to foreign libraries.
// Terminology is inspired from .net HRESULT:
// https://learn.microsoft.com/en-us/dotnet/framework/interop/how-to-map-hresults-and-exceptions
enum error_codes {
  // NullReferenceException
  C11_E_POINTER = (int)0x80000000,
  // NotSupportedException
  C11_E_NOTSUPPORTED,
  // ArgumentException
  C11_E_INVALIDARG,
  // ArgumentOutOfRangeException
  C11_E_ARGUMENTOUTOFRANGE,
  // ObjectDisposedException
  C11_E_OBJECTDISPOSED,
  // IOException
  C11_E_IO
};

// 'c11' is the namespace, 'stream' is the type:
struct c11_stream;
// function declaration for stream interface
// all return types are signed to allow error codes to be returned
typedef buf_size (*read_fn)(struct c11_stream* self, byte* buffer,
                            buf_size count);
typedef buf_size (*write_fn)(struct c11_stream* self, const byte* buffer,
                             buf_size count);
typedef stream_offset (*seek_fn)(struct c11_stream* self, stream_offset off,
                                 seek_dir dir);
typedef int (*flush_fn)(struct c11_stream* self);
typedef stream_length (*trunc_fn)(struct c11_stream* self, stream_length size);

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

#define CHECK_ARGS(s, e1, f, e2) \
  do {                           \
    if (!(s)) {                  \
      return (e1);               \
    }                            \
    if (!((s)->f)) {             \
      return (e2);               \
    }                            \
  } while (0)

static inline buf_size c11_stream_read(struct c11_stream* c11_stream,
                                       byte* buffer, const buf_size count) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, read, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->read(c11_stream, buffer, count);
}

static inline buf_size c11_stream_write(struct c11_stream* c11_stream,
                                        const byte* buffer,
                                        const buf_size count) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, write, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->write(c11_stream, buffer, count);
}

static inline stream_offset c11_stream_seek(struct c11_stream* c11_stream,
                                            const stream_offset off,
                                            const seek_dir dir) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, seek, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->seek(c11_stream, off, dir);
}

static inline int c11_stream_flush(struct c11_stream* c11_stream) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, flush, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->flush(c11_stream);
}

static inline stream_length c11_stream_trunc(struct c11_stream* c11_stream,
                                             const stream_length size) {
  CHECK_ARGS(c11_stream, C11_E_POINTER, trunc, C11_E_NOTSUPPORTED);
  // else
  return c11_stream->trunc(c11_stream, size);
}

#undef CHECK_ARGS
#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // CXX11_BINDINGS_STREAM_H
