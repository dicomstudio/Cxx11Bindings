#include "public.h"
#include "std_streambuf.hxx"

#include <cassert>
#include <cstdio>
#include <fstream>

#ifndef _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#endif

namespace cxx11 {
struct filebuf {
  // Order is important:
  std::filebuf fb{};
  // FIXME: no portable way to store the open mode
  // TODO: deprecated, should be using std::openmode
  std::ios_base::openmode open_mode{};
};
}  // namespace cxx11

extern "C" {
cxx11_filebuf* cxx11_filebuf_create1(const char* path, const int file_mode,
                                     const int file_access,
                                     const int file_share) {
  (void)file_mode;
  (void)file_share;
  auto cxx11_fb = new (std::nothrow) cxx11::filebuf;
  if (cxx11_fb == nullptr) {
    return nullptr;
  }
  std::filebuf* filebuf = &cxx11_fb->fb;
  auto& open_mode = cxx11_fb->open_mode;
  switch (file_access) {
    case 1:  // FileRead
      open_mode = std::ios::in;
      break;
    case 2:  // FileWrite
      open_mode = std::ios::out;
      break;
    case 3:  // FileAccess.ReadWrite
      open_mode = std::ios::in | std::ios::out;
      break;
    default:
      assert(0);
      delete cxx11_fb;
      return nullptr;
  }
  if (filebuf->open(path, cxx11_fb->open_mode)) {
    // File opened successfully
    return reinterpret_cast<cxx11_filebuf*>(cxx11_fb);
  }
  // Failed to open file
  delete cxx11_fb;
  return nullptr;
}

cxx11_filebuf* cxx11_filebuf_create2(const wchar_t* path, const int file_mode,
                                     const int file_access,
                                     const int file_share) {
  (void)file_mode;
  (void)file_share;
  auto cxx11_fb = new (std::nothrow) cxx11::filebuf;
  if (cxx11_fb == nullptr) {
    return nullptr;
  }
  std::filebuf* filebuf = &cxx11_fb->fb;
  auto& open_mode = cxx11_fb->open_mode;
  switch (file_access) {
    case 1:  // FileRead
      open_mode = std::ios::in;
      break;
    case 2:  // FileWrite
      open_mode = std::ios::out;
      break;
    case 3:  // FileAccess.ReadWrite
      open_mode = std::ios::in | std::ios::out;
      break;
    default:
      assert(0);
      delete cxx11_fb;
      return nullptr;
  }
#ifdef _MSC_VER
  if (filebuf->open(path, cxx11_fb->open_mode)) {
    // File opened successfully
    return reinterpret_cast<cxx11_filebuf*>(cxx11_fb);
  }
#endif
  // Failed to open file
  delete cxx11_fb;
  return nullptr;
}

void cxx11_filebuf_delete(cxx11_filebuf* cxx11_fb) {
  delete reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
}

int cxx11_filebuf_read(cxx11_filebuf* cxx11_fb, char* buffer,
                       const int buffer_length, const int offset,
                       const int count) {
  (void)buffer_length;
  (void)offset;
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_read(filebuf, buffer, count);
}

int cxx11_filebuf_write(cxx11_filebuf* cxx11_fb, const char* buffer,
                        const int buffer_length, const int offset,
                        const int count) {
  (void)buffer_length;
  (void)count;
  (void)offset;
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_write(filebuf, buffer, count);
}

long cxx11_filebuf_seek(cxx11_filebuf* cxx11_fb, const long offset,
                        const int origin) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_seek(filebuf, offset, origin);
}

int cxx11_filebuf_flush(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_flush(filebuf);
}

long cxx11_filebuf_get_position(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_get_position(filebuf);
}

int cxx11_filebuf_set_position(cxx11_filebuf* cxx11_fb, const long position) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_set_position(filebuf, position);
}

long cxx11_filebuf_get_length(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_get_length(filebuf);
}

int cxx11_filebuf_set_length(cxx11_filebuf* cxx11_fb, const long length) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
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

int cxx11_filebuf_can_read(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  // filebuf specific:
  auto mode = cxx11_filebuf->open_mode;
  if (filebuf->is_open() && mode & std::ios::in) {
    return 0;
  }
  return -1;
}

int cxx11_filebuf_can_write(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  const auto mode = cxx11_filebuf->open_mode;
  // filebuf specific:
  if (filebuf->is_open() && mode & std::ios::out) {
    return 0;
  }
  return -1;
}

int cxx11_filebuf_can_seek(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  return std_streambuf_can_seek(filebuf);
}
}
