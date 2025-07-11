#pragma once
#include <ios>

template <typename T = std::streambuf>
static inline int std_streambuf_read(T* streambuf, char* buffer,
                                     const int count) {
  try {
    const int bytes_read = static_cast<int>(streambuf->sgetn(buffer, count));
    return bytes_read;
  } catch (...) {
    return -1;
  }
}

template <typename T = std::streambuf>
static inline int std_streambuf_write(T* streambuf, const char* buffer,
                                      const int count) {
  try {
    const int bytes_read = static_cast<int>(streambuf->sputn(buffer, count));
    return bytes_read;
  } catch (...) {
    return -1;
  }
}

template <typename T = std::streambuf>
long std_streambuf_seek(T* streambuf, const long offset, const int origin) {
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
      streambuf->pubseekoff(offset, dir, std::ios_base::in);
  return static_cast<long>(ret);
}

template <typename T = std::streambuf>
static inline int std_streambuf_flush(T* streambuf) {
  const int result = streambuf->pubsync();
  // returns 0 on success, -1 on failure
  return result;
}

template <typename T = std::streambuf>
static inline long std_streambuf_get_position(T* streambuf) {
  const std::streampos pos =
      streambuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  if (pos == std::streampos(-1)) {
    return -1;
  }
  return static_cast<long>(pos);
}

template <typename T = std::streambuf>
static inline int std_streambuf_set_position(T* streambuf,
                                             const long position) {
  const std::streampos pos =
      streambuf->pubseekpos(position, std::ios_base::in | std::ios_base::out);
  if (pos == std::streampos(-1)) {
    return -1;
  }
  return 0;
}

template <typename T = std::streambuf>
static inline int std_streambuf_can_seek(T* streambuf) {
  if (!streambuf->is_open()) {
    return -1;
  }
  const auto pos =
      streambuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  if (pos != std::streampos(-1)) {
    return 0;
  }
  return -1;
}

template <typename T = std::streambuf>
static inline long std_streambuf_get_length(T* streambuf) {
  // Save current position
  const std::streampos current =
      streambuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  if (current == std::streampos(-1)) {
    return -1;
  }
  // Seek to end to get length
  const std::streampos end =
      streambuf->pubseekoff(0, std::ios_base::end, std::ios_base::in);
  if (end == std::streampos(-1)) {
    return -1;
  }
  // Restore position
  streambuf->pubseekpos(current, std::ios_base::in);
  return static_cast<long>(end);
}
