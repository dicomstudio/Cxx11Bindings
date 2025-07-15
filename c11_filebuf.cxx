#include "public.h"
#include "std_streambuf.hxx"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <streambuf>

#ifndef _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#endif

namespace details {
class filebuf : public std::streambuf {
  FILE* file_;
  char buffer_[512];

 public:
  filebuf() : file_(nullptr) {
    setg(buffer_, buffer_, buffer_);
    setp(buffer_, buffer_ + sizeof(buffer_));
  }

  // C++
  ~filebuf() override { close(); }

  bool open(const char* filename, const char* mode) {
    file_ = std::fopen(filename, mode);
    return file_ != nullptr;
  }
#ifdef _MSC_VER
  bool open(const wchar_t* filename, const wchar_t* mode) {
    file_ = _wfopen(filename, mode);
    return file_ != nullptr;
  }
#endif

  void close() {
    sync();
    if (file_) {
      std::fclose(file_);
    }
    file_ = nullptr;
  }

  bool is_open() const { return file_ != nullptr; }

 protected:
  int_type underflow() override {
    if (!file_) {
      return traits_type::eof();
    }
    const size_t n = std::fread(buffer_, 1, sizeof(buffer_), file_);
    if (n == 0) {
      return traits_type::eof();
    }
    setg(buffer_, buffer_, buffer_ + n);
    return traits_type::to_int_type(*gptr());
  }

  int_type overflow(int_type ch = traits_type::eof()) override {
    if (!file_ || pptr() == pbase()) {
      return traits_type::eof();
    }
    const size_t n = pptr() - pbase();
    const size_t written = std::fwrite(pbase(), 1, n, file_);
    setp(buffer_, buffer_ + sizeof(buffer_));
    if (ch != traits_type::eof()) {
      *pptr() = traits_type::to_char_type(ch);
      pbump(1);
    }
    return written == n ? ch : traits_type::eof();
  }

  int sync() override {
    if (pptr() > pbase()) {
      overflow();
    }
    return 0;
  }

  pos_type seekoff(off_type off, std::ios_base::seekdir way,
                   std::ios_base::openmode which) override {
    int whence = SEEK_SET;
    if (way == std::ios_base::cur) {
      whence = SEEK_CUR;
    } else if (way == std::ios_base::end) {
      whence = SEEK_END;
    }
    if (std::fseek(file_, static_cast<long>(off), whence) != 0) {
      return {-1};
    }
    return std::ftell(file_);
  }

  pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
    if (std::fseek(file_, static_cast<long>(pos), SEEK_SET) != 0) {
      return {-1};
    }
    return std::ftell(file_);
  }
};
}  // namespace details

namespace c11 {
struct filebuf {
  // Order is important:
  details::filebuf fb{};
  // FIXME: no portable way to store the open mode
  std::string open_mode{};
};
}  // namespace c11

extern "C" {
c11_filebuf* c11_filebuf_create1(const char* path, const int file_mode,
                                 const int file_access, const int file_share) {
  (void)file_mode;
  (void)file_share;
  const auto c11_fb = new (std::nothrow) c11::filebuf;
  if (c11_fb == nullptr) {
    return nullptr;
  }
  details::filebuf* filebuf = &c11_fb->fb;
  auto& open_mode = c11_fb->open_mode;
  switch (file_access) {
    case 1:  // FileRead
      open_mode = "r";
      break;
    case 2:  // FileWrite
      open_mode = "w";
      break;
    case 3:  // FileAccess.ReadWrite
      open_mode = "rw";
      break;
    default:
      assert(0);
      delete c11_fb;
      return nullptr;
  }
  if (filebuf->open(path, c11_fb->open_mode.c_str())) {
    // File opened successfully
    return reinterpret_cast<c11_filebuf*>(c11_fb);
  }
  // Failed to open file
  delete c11_fb;
  return nullptr;
}

c11_filebuf* c11_filebuf_create2(const wchar_t* path, const int file_mode,
                                 const int file_access, const int file_share) {
  (void)file_mode;
  (void)file_share;
  const auto c11_fb = new (std::nothrow) c11::filebuf;
  if (c11_fb == nullptr) {
    return nullptr;
  }
  details::filebuf* filebuf = &c11_fb->fb;
  auto& open_mode = c11_fb->open_mode;
  switch (file_access) {
    case 1:  // FileRead
      open_mode = "r";
      break;
    case 2:  // FileWrite
      open_mode = "w";
      break;
    case 3:  // FileAccess.ReadWrite
      open_mode = "rw";
      break;
    default:
      assert(0);
      delete c11_fb;
      return nullptr;
  }
#ifdef _MSC_VER
  // Overestimate number of code points.
  std::wstring ws(open_mode.size(), L' ');
  // Shrink to fit.
  ws.resize(std::mbstowcs(&ws[0], open_mode.c_str(), open_mode.size()));
  if (filebuf->open(path, ws.c_str())) {
    // File opened successfully
    return reinterpret_cast<c11_filebuf*>(c11_fb);
  }
#endif
  // Failed to open file
  delete c11_fb;
  return nullptr;
}

void c11_filebuf_delete(c11_filebuf* c11_fb) {
  delete reinterpret_cast<c11::filebuf*>(c11_fb);
}

int c11_filebuf_read(c11_filebuf* c11_fb, char* buffer, const int buffer_length,
                     const int offset, const int count) {
  (void)buffer_length;
  (void)offset;
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_read(filebuf, buffer, count);
}

int c11_filebuf_write(c11_filebuf* c11_fb, const char* buffer,
                      const int buffer_length, const int offset,
                      const int count) {
  (void)buffer_length;
  (void)count;
  (void)offset;
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_write(filebuf, buffer, count);
}

long c11_filebuf_seek(c11_filebuf* c11_fb, const long offset,
                      const int origin) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_seek(filebuf, offset, origin);
}

int c11_filebuf_flush(c11_filebuf* c11_fb) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_flush(filebuf);
}

long c11_filebuf_get_position(c11_filebuf* c11_fb) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_get_position(filebuf);
}

int c11_filebuf_set_position(c11_filebuf* c11_fb, const long position) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_set_position(filebuf, position);
}

long c11_filebuf_get_length(c11_filebuf* c11_fb) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_get_length(filebuf);
}

int c11_filebuf_set_length(c11_filebuf* c11_fb, const long length) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  // filebuf specific:
#ifdef _MSC_VER
  (void)filebuf;
  (void)length;
  // FIXME on windows we cannot access the FILE* from std::filebuf
  // we should eventually do the opposite and construct the std::filebuf from
  // the FILE* directly:
  FILE* fp = nullptr;
  std::fstream stream(fp);
#else
  // linux uses ftruncate:
  // Try to get FILE* from filebuf
  FILE* fp = nullptr;  // filebuf->_M_file;  // This is non-standard, works on
                       // libstdc++
  if (!fp) {
    return -1;
  }
  int fd = fileno(fp);
  if (fd == -1) {
    return -1;
  }
  // Set the file size using ftruncate
  if (ftruncate(fd, length) == 0) {
    return 0;
  }
#endif
  return -1;
}

int c11_filebuf_can_read(c11_filebuf* c11_fb) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  const auto mode = c11_filebuf->open_mode;
  // filebuf specific:
  if (filebuf->is_open() && mode == "r") {
    return 0;
  }
  return -1;
}

int c11_filebuf_can_write(c11_filebuf* c11_fb) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  const auto mode = c11_filebuf->open_mode;
  // filebuf specific:
  if (filebuf->is_open() && mode == "w") {
    return 0;
  }
  return -1;
}

int c11_filebuf_can_seek(c11_filebuf* c11_fb) {
  const auto c11_filebuf = reinterpret_cast<c11::filebuf*>(c11_fb);
  details::filebuf* filebuf = &c11_filebuf->fb;
  return std_streambuf_can_seek(filebuf);
}
}
