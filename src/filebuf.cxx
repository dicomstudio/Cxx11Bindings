#include <cxx11bindings/filebuf.hxx>

#include "stream_interface_impl.h"
#ifndef _WIN32
#include <climits>
#include <codecvt>
#include <locale>
#endif
#include <fstream>
#include <memory>
#include <new>
#ifdef _WIN32
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
  std::string m(mode);
  std::ios::openmode binary = std::ios::openmode(0);
  if (!m.empty() && m.back() == 'b') {
    binary = std::ios::binary;
    m.pop_back();
  }
  if (m == "r") return std::ios::in | binary;
  if (m == "r+") return std::ios::in | std::ios::out | binary;
  if (m == "w") return std::ios::out | std::ios::trunc | binary;
  if (m == "w+") return std::ios::in | std::ios::out | std::ios::trunc | binary;
  if (m == "a") return std::ios::out | std::ios::app | binary;
  if (m == "a+") return std::ios::in | std::ios::out | std::ios::app | binary;
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
  return C11_E_OUTOFMEMORY;
}
}  // namespace

int cxx11_file_stream_create1(c11_stream** p_self, const char* filename,
                              const char* mode) {
  if (!p_self) return C11_E_POINTER;
  *p_self = nullptr;
  if (filename && mode) {
    try {
      const std::ios::openmode open_mode = file_mode_to_ios_flags(mode);
      std::unique_ptr<std::filebuf> fb(new std::filebuf());
#ifdef _WIN32
      const int length =
          MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1,
                              nullptr, 0);
      if (length == 0) return C11_E_INVALIDARG;
      std::wstring wide_filename(static_cast<std::size_t>(length), L'\0');
      if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1,
                              &wide_filename[0], length) == 0) {
        return C11_E_INVALIDARG;
      }
      fb->open(wide_filename.c_str(), open_mode);
#else
      fb->open(filename, open_mode);
#endif
      if (fb->is_open()) {
        const int ret = file_stream_init(p_self, fb.get(), true);
        if (ret == 0) fb.release();
        return ret;
      }
      return C11_E_IO;
    } catch (const std::bad_alloc&) {
      return C11_E_OUTOFMEMORY;
    } catch (...) {
      return C11_E_IO;
    }
  }
  // else
  return C11_E_INVALIDARG;
}

#ifdef _WIN32
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
  if (!p_self) return C11_E_POINTER;
  *p_self = nullptr;
  if (wfilename && wmode) {
    try {
      std::unique_ptr<std::filebuf> fb(new std::filebuf());
#ifdef _WIN32
      char mode[16];
      if (wchar_to_ascii(wmode, mode, sizeof mode)) {
        const std::ios::openmode open_mode = file_mode_to_ios_flags(mode);
        fb->open(wfilename, open_mode);
      }
#else
#if WCHAR_MAX <= 0xffff
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
#else
      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
#endif
      std::string filename;
      std::string mode;
      try {
        filename = converter.to_bytes(wfilename);
        if (converter.converted() !=
            std::char_traits<wchar_t>::length(wfilename)) {
          return C11_E_INVALIDARG;
        }
        mode = converter.to_bytes(wmode);
        if (converter.converted() != std::char_traits<wchar_t>::length(wmode)) {
          return C11_E_INVALIDARG;
        }
      } catch (const std::range_error&) {
        return C11_E_INVALIDARG;
      }
      fb->open(filename.c_str(), file_mode_to_ios_flags(mode.c_str()));
#endif
      if (fb->is_open()) {
        const int ret = file_stream_init(p_self, fb.get(), true);
        if (ret == 0) fb.release();
        return ret;
      }
      return C11_E_IO;
    } catch (const std::bad_alloc&) {
      return C11_E_OUTOFMEMORY;
    } catch (...) {
      return C11_E_IO;
    }
  }
  // else
  return C11_E_INVALIDARG;
}

int cxx11_file_stream_destroy(c11_stream* self) {
  if (self) {
    const auto fs = reinterpret_cast<filebuf_stream*>(self);
    int ret = 0;
    if (fs->is_created) {
      try {
        if (!static_cast<std::filebuf*>(fs->file)->close()) {
          ret = C11_E_IO;
        }
      } catch (...) {
        // std::filebuf::close can propagate exceptions from its locale facets.
        ret = C11_E_IO;
      }
    }
    delete fs;
    return ret;
  }
  return C11_E_INVALIDARG;
}

int cxx11_file_stream_init(c11_stream** p_self, std::streambuf* sb) {
  return file_stream_init(p_self, sb, false);
}

#ifdef __cplusplus
}  //  extern "C"
#endif
