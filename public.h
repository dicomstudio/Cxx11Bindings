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
struct std_streambuf;

CXX11_SHARP_EXPORT std_streambuf* cxx11_managed_streambuf_create(
    read_func read_func, write_func write_func, flush_func flush_func,
    seek_func seek_func, char* data, int size);
CXX11_SHARP_EXPORT void cxx11_managed_streambuf_delete(
    std_streambuf* streambuf);
CXX11_SHARP_EXPORT int cxx11_managed_streambuf_read_into(
    std_streambuf* src_streambuf, char* buffer, int count);
CXX11_SHARP_EXPORT int cxx11_managed_streambuf_write_into(
    std_streambuf* dst_streambuf, const char* buffer, int count);
CXX11_SHARP_EXPORT int cxx11_managed_streambuf_flush(
    std_streambuf* dst_streambuf);

CXX11_SHARP_EXPORT long cxx11_managed_streambuf_test_size(
    std_streambuf* dst_streambuf);

#ifdef __cplusplus
}  // end extern "C"
#endif
