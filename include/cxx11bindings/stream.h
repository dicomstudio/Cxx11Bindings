#ifndef CXX11_BINDINGS_STREAM_H
#define CXX11_BINDINGS_STREAM_H

#include <stddef.h>  // size_t
#include <stdint.h>  // require c99
#include <string.h>

#define CXX11BINDINGS_VERSION_MAJOR 3
#define CXX11BINDINGS_VERSION_MINOR 2
#define CXX11BINDINGS_VERSION_PATCH 1

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef CXX11_BINDINGS_BUILD
#define CXX11_BINDINGS_EXPORT __declspec(dllexport)
#else
#define CXX11_BINDINGS_EXPORT __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define CXX11_BINDINGS_EXPORT __attribute__((visibility("default")))
#else
#define CXX11_BINDINGS_EXPORT
#endif

#if defined(__GNUC__) || defined(__clang__)
#define CXX11_CHECK_RETURN __attribute__((__warn_unused_result__))
#else
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
  C11_E_POINTER = -2147483647 - 1, // 0x80000000
  // NotSupportedException
  C11_E_NOTSUPPORTED,
  // ArgumentException
  C11_E_INVALIDARG,
  // ArgumentOutOfRangeException
  C11_E_ARGUMENTOUTOFRANGE,
  // ObjectDisposedException
  C11_E_OBJECTDISPOSED,
  // IOException
  C11_E_IO,
  C11_E_INVALIDOPERATION,
  C11_E_UNAUTHORIZED,
  C11_E_OUTOFMEMORY,
  C11_E_TIMEOUT,
  C11_E_CANCELED,
  C11_E_FILENOTFOUND,
  C11_E_DIRECTORYNOTFOUND,
  C11_E_PATHTOOLONG,
  C11_E_ENDOFSTREAM,
  C11_E_ARGUMENTNULL,
  C11_E_NOTIMPLEMENTED,
  C11_E_UNMAPPED
};

// 'c11' is the namespace, 'stream' is the type:
struct c11_stream;
// The stream interface is synchronous: each callback completes its operation
// and returns its result before the call returns. Read and write callbacks
// must not access the supplied buffer after returning. Calls may block.
// No asynchronous completion or thread-safety guarantee is provided.
// function declaration for stream interface
// all return types are signed to allow error codes to be returned
// Return the number of bytes read (0..count), or a negative error if no
// bytes were read. A short read may include progress made before an error.
typedef buf_size (*read_fn)(struct c11_stream* self, byte* buffer,
                            buf_size count);
// Return the number of bytes written (0..count), or a negative error if no
// bytes were written. A short write may include progress made before an error.
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
  if (dir != seek_beg && dir != seek_cur && dir != seek_end) {
    return C11_E_INVALIDARG;
  }
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

// Fixed-capacity, caller-owned memory. Keep both this object and its data
// alive while the stream is in use, and do not copy an initialized object.
struct c11_memory_stream {
  struct c11_stream super;
  byte* data;
  stream_length capacity;
  stream_length length;
  stream_offset position;
};

static inline buf_size c11_memory_stream_read(struct c11_stream* self,
                                               byte* buffer, buf_size count) {
  struct c11_memory_stream* memory = (struct c11_memory_stream*)self;
  stream_length available;
  if (!memory || !buffer) return C11_E_POINTER;
  if (count < 0) return C11_E_INVALIDARG;
  if (memory->position >= memory->length) return 0;
  available = memory->length - memory->position;
  if (available < count) count = (buf_size)available;
  memmove(buffer, memory->data + (size_t)memory->position, (size_t)count);
  memory->position += count;
  return count;
}

static inline buf_size c11_memory_stream_write(struct c11_stream* self,
                                                const byte* buffer,
                                                buf_size count) {
  struct c11_memory_stream* memory = (struct c11_memory_stream*)self;
  stream_length available;
  if (!memory || !buffer) return C11_E_POINTER;
  if (count < 0) return C11_E_INVALIDARG;
  available = memory->capacity - memory->position;
  if (available < count) count = (buf_size)available;
  if (count == 0) return 0;
  // Copy first so a source in the gap is not overwritten by zero filling.
  memmove(memory->data + (size_t)memory->position, buffer, (size_t)count);
  if (memory->position > memory->length) {
    memset(memory->data + (size_t)memory->length, 0,
           (size_t)(memory->position - memory->length));
  }
  memory->position += count;
  if (memory->position > memory->length) memory->length = memory->position;
  return count;
}

static inline stream_offset c11_memory_stream_seek(struct c11_stream* self,
                                                    stream_offset off,
                                                    seek_dir dir) {
  struct c11_memory_stream* memory = (struct c11_memory_stream*)self;
  stream_offset base;
  if (!memory) return C11_E_POINTER;
  switch (dir) {
    case seek_beg: base = 0; break;
    case seek_cur: base = memory->position; break;
    case seek_end: base = memory->length; break;
    default: return C11_E_INVALIDARG;
  }
  if (off < -base || off > memory->capacity - base)
    return C11_E_INVALIDARG;
  memory->position = base + off;
  return memory->position;
}

static inline int c11_memory_stream_flush(struct c11_stream* self) {
  return self ? 0 : C11_E_POINTER;
}

static inline stream_length c11_memory_stream_trunc(struct c11_stream* self,
                                                     stream_length size) {
  struct c11_memory_stream* memory = (struct c11_memory_stream*)self;
  if (!memory) return C11_E_POINTER;
  if (size < 0 || size > memory->capacity) return C11_E_INVALIDARG;
  if (size > memory->length) {
    memset(memory->data + (size_t)memory->length, 0,
           (size_t)(size - memory->length));
  }
  memory->length = size;
  return size;
}

// data may be null only for zero capacity. length is the initially valid data
// size. Seeking is limited to [0, capacity]; truncate preserves position.
static inline int c11_memory_stream_init(struct c11_memory_stream* self,
                                         byte* data, stream_length capacity,
                                         stream_length length) {
  if (!self) return C11_E_POINTER;
  if (capacity < 0 || length < 0 || length > capacity ||
      (uintmax_t)capacity > (uintmax_t)SIZE_MAX)
    return C11_E_INVALIDARG;
  if (!data && capacity != 0) return C11_E_POINTER;
  self->super.read = c11_memory_stream_read;
  self->super.write = c11_memory_stream_write;
  self->super.seek = c11_memory_stream_seek;
  self->super.flush = c11_memory_stream_flush;
  self->super.trunc = c11_memory_stream_trunc;
  self->data = data;
  self->capacity = capacity;
  self->length = length;
  self->position = 0;
  return 0;
}

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // CXX11_BINDINGS_STREAM_H
