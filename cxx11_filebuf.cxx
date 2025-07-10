#include "public.h"

#include <cassert>
#include <cstdio>
#include <fstream>

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
  const std::streamsize bytes_read = filebuf->sgetn(buffer, count);
  return static_cast<int>(bytes_read);
}

int cxx11_filebuf_write(cxx11_filebuf* cxx11_fb, const char* buffer,
                        const int buffer_length, const int offset,
                        const int count) {
  (void)buffer_length;
  (void)count;
  (void)offset;
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  const std::streamsize bytes_read = filebuf->sputn(buffer, count);
  return static_cast<int>(bytes_read);
}

long cxx11_filebuf_seek(cxx11_filebuf* cxx11_fb, const long offset,
                        const int origin) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  std::ios_base::seekdir dir;
  // Begin 0
  // Current 1
  // End 2
  switch (origin) {
    case 0:
      dir = std::ios_base::beg;
      break;
    case 1:
      dir = std::ios_base::cur;
      break;
    case 2:
      dir = std::ios_base::end;
      break;
    default:
      return -1;  // Invalid origin
  }

  const std::streamoff ret =
      filebuf->pubseekoff(offset, dir, std::ios_base::in);
  return static_cast<long>(ret);
}

int cxx11_filebuf_flush(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  const int result = filebuf->pubsync();  // returns 0 on success, -1 on failure
  return result;
}

long cxx11_filebuf_get_position(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  const std::streampos pos =
      filebuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  if (pos == std::streampos(-1)) {
    return -1;
  }
  return static_cast<long>(pos);
}

int cxx11_filebuf_set_position(cxx11_filebuf* cxx11_fb, const long position) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  const std::streampos pos =
      filebuf->pubseekpos(position, std::ios_base::in | std::ios_base::out);
  if (pos == std::streampos(-1)) {
    return -1;
  }
  return 0;
}

long cxx11_filebuf_get_length(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  // Save current position
  const std::streampos current =
      filebuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  if (current == std::streampos(-1)) {
    return -1;
  }
  // Seek to end to get length
  const std::streampos end =
      filebuf->pubseekoff(0, std::ios_base::end, std::ios_base::in);
  if (end == std::streampos(-1)) {
    return -1;
  }
  // Restore position
  filebuf->pubseekpos(current, std::ios_base::in);
  return static_cast<long>(end);
}

int cxx11_filebuf_set_length(cxx11_filebuf*, long) {
  FILE* fp = nullptr;
#ifdef _MSC_VER
  std::fstream stream(fp);
#endif
  return -1;
}

int cxx11_filebuf_can_read(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
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
  if (filebuf->is_open() && mode & std::ios::out) {
    return 0;
  }
  return -1;
}

int cxx11_filebuf_can_seek(cxx11_filebuf* cxx11_fb) {
  const auto cxx11_filebuf = reinterpret_cast<cxx11::filebuf*>(cxx11_fb);
  std::filebuf* filebuf = &cxx11_filebuf->fb;
  if (!filebuf->is_open()) {
    return -1;
  }
  const auto pos =
      filebuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  if (pos != std::streampos(-1)) {
    return 0;
  }
  return -1;
}
}
