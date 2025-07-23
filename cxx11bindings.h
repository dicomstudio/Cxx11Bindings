#pragma once
#include <stdint.h>
#ifdef __GNUC__
#define CXX11_BINDINGS_EXPORT __attribute__((visibility("default")))
#else
#define CXX11_BINDINGS_EXPORT __declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// https://stackoverflow.com/questions/29631692/self-referencing-class-concrete-python-class-from-c-interface

// On  Linux,  write()  (and similar system calls) will transfer at most
// 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes actually
// transferred.
typedef uint8_t byte;
// stream offset (aka off_t)
typedef int64_t offset;
// buffer size (aka size_t)
typedef uint32_t size;
// stream length (aka off_t)
typedef uint64_t length;
// seek set/cur/end type:
typedef int seek_dir;

enum seek_dirs {
  seek_beg = 0,
  seek_cur = 1,
  seek_end = 2,
};

typedef int (*read_fn)(byte* buffer, size count);
typedef int (*write_fn)(const byte* buffer, size count);
typedef offset (*seek_fn)(offset off, seek_dir dir);
typedef int (*flush_fn)();
typedef int (*trunc_fn)(length size);

// opaque type:
struct c11_stream;
CXX11_BINDINGS_EXPORT struct c11_stream* c11_stream_create(
    read_fn read, write_fn write, seek_fn seek, flush_fn flush, trunc_fn trunc);
CXX11_BINDINGS_EXPORT int c11_stream_delete(struct c11_stream* c11_stream);

// opaque std::streambuf with c++11 ABI:
struct cxx11_streambuf;
CXX11_BINDINGS_EXPORT struct cxx11_streambuf* cxx11_streambuf_create(
    read_fn read, write_fn write, seek_fn seek, flush_fn flush);
CXX11_BINDINGS_EXPORT struct cxx11_streambuf* cxx11_streambuf_create_buffer(
    read_fn read, write_fn write, seek_fn seek, flush_fn flush, byte* buf,
    size count);
CXX11_BINDINGS_EXPORT int cxx11_streambuf_delete(
    struct cxx11_streambuf* cxx11_streambuf);

#ifdef __cplusplus
}  // end extern "C"
#endif
