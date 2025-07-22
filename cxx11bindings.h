#pragma once
#include <stdint.h>
#include <wchar.h>
#ifdef __GNUC__
#define CXX11_BINDINGS_EXPORT __attribute__((visibility("default")))
#else
#define CXX11_BINDINGS_EXPORT __declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// https://stackoverflow.com/questions/29631692/self-referencing-class-concrete-python-class-from-c-interface

typedef uint8_t byte;
struct cxx11_stream;

enum seek_dir {
  seek_beg = 0,
  seek_cur = 1,
  seek_end = 2,
};

typedef int (*read_fn)(struct cxx11_stream* self, byte* buffer, int size);
typedef int (*write_fn)(struct cxx11_stream* self, const byte* buffer,
                        int size);
typedef int64_t (*seek_fn)(struct cxx11_stream* self, int64_t off,
                           int seek_dir);
typedef int (*flush_fn)(struct cxx11_stream* self);

struct cxx11_stream {
  read_fn read;
  write_fn write;
  seek_fn seek;
  flush_fn flush;
};

CXX11_BINDINGS_EXPORT struct cxx11_stream* cxx11_stream_create(read_fn read,
                                                               write_fn write,
                                                               flush_fn flush,
                                                               seek_fn seek);
CXX11_BINDINGS_EXPORT int cxx11_stream_delete(
    struct cxx11_stream* cxx11_stream);

#ifdef __cplusplus
}  // end extern "C"
#endif
