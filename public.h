#pragma once
#include <wchar.h>
#ifdef __GNUC__
#define CXX11_SHARP_EXPORT __attribute__((visibility("default")))
#else
#define CXX11_SHARP_EXPORT __declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*read_func)();
typedef int (*write_func)(int);
typedef long (*seek_func)(long, int);
typedef int (*flush_func)();

// opaque type:
struct cxx11_streambuf;

CXX11_SHARP_EXPORT cxx11_streambuf* cxx11_managed_streambuf_create(
    read_func read_func, write_func write_func, flush_func flush_func,
    seek_func seek_func, char* data, int size);
CXX11_SHARP_EXPORT void cxx11_managed_streambuf_delete(
    cxx11_streambuf* streambuf);
CXX11_SHARP_EXPORT int cxx11_managed_streambuf_read_into(
    cxx11_streambuf* src_streambuf, char* buffer, int count);
CXX11_SHARP_EXPORT int cxx11_managed_streambuf_write_into(
    cxx11_streambuf* dst_streambuf, const char* buffer, int count);
CXX11_SHARP_EXPORT int cxx11_managed_streambuf_flush(
    cxx11_streambuf* dst_streambuf);
CXX11_SHARP_EXPORT long cxx11_managed_streambuf_test_size(
    cxx11_streambuf* dst_streambuf);

struct cxx11_filebuf;
CXX11_SHARP_EXPORT cxx11_filebuf* cxx11_filebuf_create1(const char* path,
                                                        int file_mode,
                                                        int file_access,
                                                        int file_share);
CXX11_SHARP_EXPORT cxx11_filebuf* cxx11_filebuf_create2(const wchar_t* path,
                                                        int file_mode,
                                                        int file_access,
                                                        int file_share);
CXX11_SHARP_EXPORT void cxx11_filebuf_delete(cxx11_filebuf* cxx11_fb);
CXX11_SHARP_EXPORT int cxx11_filebuf_read(cxx11_filebuf* filebuf, char* buffer,
                                          int buffer_length, int offset,
                                          int count);
CXX11_SHARP_EXPORT int cxx11_filebuf_write(cxx11_filebuf* filebuf,
                                           const char* buffer,
                                           int buffer_length, int offset,
                                           int count);
CXX11_SHARP_EXPORT long cxx11_filebuf_seek(cxx11_filebuf* cxx11_fb, long offset,
                                           int origin);
CXX11_SHARP_EXPORT int cxx11_filebuf_flush(cxx11_filebuf* cxx11_fb);
CXX11_SHARP_EXPORT long cxx11_filebuf_get_position(cxx11_filebuf* cxx11_fb);
CXX11_SHARP_EXPORT int cxx11_filebuf_set_position(cxx11_filebuf* cxx11_fb,
                                                  long position);
CXX11_SHARP_EXPORT long cxx11_filebuf_get_length(cxx11_filebuf* cxx11_fb);
CXX11_SHARP_EXPORT int cxx11_filebuf_set_length(cxx11_filebuf* filebuf,
                                                long length);
CXX11_SHARP_EXPORT int cxx11_filebuf_can_read(cxx11_filebuf* cxx11_fb);
CXX11_SHARP_EXPORT int cxx11_filebuf_can_write(cxx11_filebuf* cxx11_fb);
CXX11_SHARP_EXPORT int cxx11_filebuf_can_seek(cxx11_filebuf* cxx11_fb);

#ifdef __cplusplus
}  // end extern "C"
#endif
