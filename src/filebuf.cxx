#include <cxx11bindings/filebuf.hxx>

#include "stream_interface_impl.h"
#include <fstream>
#ifdef _MSC_VER
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct filebuf_stream {
  c11_stream super;
  /* data */
  std::streambuf* file;
  bool is_created;

  explicit filebuf_stream(std::streambuf* sb, const bool created)
      : super(), file(sb), is_created(created) {}

  ~filebuf_stream() {
    if (is_created) {
      delete file;
    }
  }
};

static buf_size file_read(c11_stream* self, byte* buffer,
                          const buf_size count) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::streambuf* file = fs->file;
  const buf_size read = static_cast<buf_size>(
      file->sgetn(reinterpret_cast<char*>(buffer), count));
  // FIXME: cannot tell when an error occurs or just plain short-read.
  return read;
}

static buf_size file_write(c11_stream* self, const byte* buffer,
                           const buf_size count) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::streambuf* file = fs->file;
  const buf_size written = static_cast<buf_size>(
      file->sputn(reinterpret_cast<const char*>(buffer), count));
  if (written != count) {
    return C11_E_IO;
  }
  return written;
}

static stream_offset file_seek(c11_stream* self, const stream_offset offset,
                               const seek_dir dir) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::streambuf* file = fs->file;
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
  const std::streampos pos = file->pubseekoff(offset, seekdir);
  return pos;
}

static int file_flush(c11_stream* self) {
  const auto fs = reinterpret_cast<filebuf_stream*>(self);
  std::streambuf* file = fs->file;
  // Flush the filebuf
  if (file->pubsync() == 0) {
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

int file_stream_init(c11_stream** p_self, std::streambuf* streambuf,
                     const bool created) {
  const auto self = new (std::nothrow) filebuf_stream(streambuf, created);
  if (self) {
    *p_self = &self->super;
    c11_stream* stream = &self->super;
    stream->read = file_read;
    stream->write = file_write;
    stream->seek = file_seek;
    stream->flush = file_flush;
    stream->trunc = nullptr;  // not implemented for std::filebuf
    // success
    return 0;
  }
  *p_self = nullptr;
  return C11_E_POINTER;
}
}  // namespace

int cxx11_file_stream_create1(c11_stream** p_self, const char* filename,
                              const char* mode) {
  if (filename && mode) {
    const std::ios::openmode open_mode = file_mode_to_ios_flags(mode);
    const auto fb = new std::filebuf();
    fb->open(filename, open_mode);
    if (fb->is_open()) {
      return file_stream_init(p_self, fb, true);
    }
    // else
    return C11_E_IO;
  }
  // else
  return C11_E_INVALIDARG;
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
int cxx11_file_stream_create2(c11_stream** p_self, const wchar_t* wfilename,
                              const wchar_t* wmode) {
  if (wfilename && wmode) {
    const auto fb = new std::filebuf();
#ifdef _MSC_VER
    char mode[16];
    if (wchar_to_ascii(wmode, mode, sizeof mode)) {
      const std::ios::openmode open_mode = file_mode_to_ios_flags(mode);
      fb->open(wfilename, open_mode);
    }
#endif
    if (fb->is_open()) {
      return file_stream_init(p_self, fb, true);
    }
    // else
    return C11_E_IO;
  }
  // else
  return C11_E_INVALIDARG;
}

int cxx11_file_stream_destroy(c11_stream* self) {
  if (self) {
    const auto fs = reinterpret_cast<filebuf_stream*>(self);
    delete fs;
    return 0;
  }
  return C11_E_INVALIDARG;
}

int cxx11_file_stream_init(c11_stream** p_self, std::streambuf* sb) {
  return file_stream_init(p_self, sb, false);
}

#ifdef __cplusplus
}  //  extern "C"
#endif
