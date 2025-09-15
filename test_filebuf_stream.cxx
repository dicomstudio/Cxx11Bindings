#include "stream_interface_impl.h"

#include <fstream>
#ifdef _MSC_VER
#include <windows.h>
#endif

struct filebuf_stream {
  c11_stream super;
  /* data */
  std::filebuf file{};
};
#ifdef __cplusplus
extern "C" {
#endif

CXX11_BINDINGS_EXPORT int test_cxx11_file_stream_create1(c11_stream** p_self,
                                                         const char* filename,
                                                         const char* mode);
CXX11_BINDINGS_EXPORT int test_cxx11_file_stream_create2(
    c11_stream** p_self, const wchar_t* filename, const wchar_t* wmode);
CXX11_BINDINGS_EXPORT int test_cxx11_file_stream_destroy(c11_stream* self);

static buf_size my_read(c11_stream* self, byte* buffer, const buf_size count) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::filebuf& file = fs->file;
  const buf_size read =
      static_cast<buf_size>(file.sgetn(reinterpret_cast<char*>(buffer), count));
  // cannot tell when an error occurs or just plain short-read.
  return read;
}

static buf_size my_write(c11_stream* self, const byte* buffer,
                         const buf_size count) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::filebuf& file = fs->file;
  const buf_size written = static_cast<buf_size>(
      file.sputn(reinterpret_cast<const char*>(buffer), count));
  if (written != count) {
    return C11_E_IO;
  }
  return written;
}

static stream_offset my_seek(c11_stream* self, const stream_offset offset,
                             const seek_dir dir) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::filebuf& file = fs->file;
  std::ios::seekdir seekdir;
  switch (dir) {
    case seek_beg:
      seekdir = std::ios::beg;
      break;
    case seek_cur:
      seekdir = std::ios::cur;
      break;
    case seek_end:
      seekdir = std::ios::end;
      break;
    default:
      return C11_E_INVALIDARG;
  }
  const std::streampos pos = file.pubseekoff(offset, seekdir);
  return pos;
}

static int my_flush(c11_stream* self) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::filebuf& file = fs->file;
  // Flush the filebuf
  if (file.pubsync() == 0) {
    return 0;
  }
  // Flush failed
  return -1;
}

namespace {
std::ios::openmode file_mode_to_ios_flags(const char* mode) {
  const std::string m(mode);
  if (m == "r") return std::ios::in;
  if (m == "r+") return std::ios::in | std::ios::out;
  if (m == "w") return std::ios::out | std::ios::trunc;
  if (m == "w+") return std::ios::in | std::ios::out | std::ios::trunc;
  if (m == "a") return std::ios::out | std::ios::app;
  if (m == "a+") return std::ios::in | std::ios::out | std::ios::app;
  // Add more cases as needed
  return std::ios::openmode(0);  // Unknown mode
}
}  // namespace

int test_cxx11_file_stream_create1(c11_stream** p_self, const char* filename,
                                   const char* mode) {
  const auto self = new filebuf_stream;
  if (self) {
    *p_self = &self->super;
    c11_stream* stream = &self->super;
    stream->read = my_read;
    stream->write = my_write;
    stream->seek = my_seek;
    stream->flush = my_flush;
    stream->trunc = nullptr;  // not implemented for std::filebuf
    const std::ios::openmode open_mode = file_mode_to_ios_flags(mode);
    std::filebuf& file = self->file;
    file.open(filename, open_mode);
    if (file.is_open()) {
      return 0;
    }
    // else error:
    delete self;
  }
  *p_self = nullptr;
  return -1;
}

#ifdef _MSC_VER
namespace {
// Converts wchar_t* to char* (ASCII). Returns true on success.
bool wchar_to_ascii(const wchar_t* wstr, char* str, const int str_size) {
  const int ret =
      WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, str_size, NULL, NULL);
  return ret > 0;
}
}  // namespace
#endif
int test_cxx11_file_stream_create2(c11_stream** p_self, const wchar_t* filename,
                                   const wchar_t* wmode) {
  const auto self = new filebuf_stream;
  if (self) {
    *p_self = &self->super;
    c11_stream* stream = &self->super;
    stream->read = my_read;
    stream->write = my_write;
    stream->seek = my_seek;
    stream->flush = my_flush;
    stream->trunc = nullptr;
    std::filebuf& file = self->file;
#ifdef _MSC_VER
    char mode[16];
    if (wchar_to_ascii(wmode, mode, sizeof mode)) {
      const std::ios::openmode open_mode = file_mode_to_ios_flags(mode);
      file.open(filename, open_mode);
    }
#endif
    if (file.is_open()) {
      return 0;
    }
    // else error:
    delete self;
  }
  *p_self = nullptr;
  return -1;
}

int test_cxx11_file_stream_destroy(c11_stream* self) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::filebuf& file = fs->file;
  const int ret = file.close() == nullptr ? -1 : 0;
  delete fs;
  return ret;
}
#ifdef __cplusplus
}  //  extern "C"
#endif
