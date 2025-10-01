// Mostly for internal testing, it provides different stream_interface
// implementation using standard API (FILE*, std::streambuf)
#pragma once
#include <cxx11bindings/streambuf.hxx>

#include "stream_interface_impl.h"
#include <ios>
#include <streambuf>
#include <string>

namespace cxx11 {
#ifdef _WIN32
// stream_interface implementation using file handle
class handle_stream final : public stream_interface {
 public:
  explicit handle_stream(const HANDLE handle) : handle_(handle) {
    if (handle == INVALID_HANDLE_VALUE) {
      throw invalid_argument();
    }
  }

  buf_size read(byte* buf, const buf_size count) override {
    const buf_size ret = read_handle(handle_, buf, count);
    throw_exception_from_negative_value(ret);
    // short-read ok:
    return ret;
  }

  buf_size write(const byte* buf, const buf_size count) override {
    const buf_size ret = write_handle(handle_, buf, count);
    throw_exception_from_negative_value(ret);
    return ret;
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    const int64_t ret64 = seek_handle(handle_, off, dir);
    throw_exception_from_negative_value(ret64);
    return ret64;
  }

  int flush() override {
    const int ret = flush_handle(handle_);
    throw_exception_from_negative_value(ret);
    return ret;
  }

  stream_length trunc(const stream_length size) override {
    const int64_t ret64 = truncate_handle(handle_, size);
    throw_exception_from_negative_value(ret64);
    return ret64;
  }

 private:
  HANDLE handle_;
};
#else
// stream_interface implementation using file descriptor
class fd_stream final : public stream_interface {
 public:
  explicit fd_stream(int fd) : fd_(fd) {
    if (fd < 0) {
      throw invalid_argument();
    }
  }

  buf_size read(byte* buf, const buf_size count) override {
    const buf_size ret = read_fd(fd_, buf, count);
    throw_exception_from_negative_value(ret);
    // short-read ok:
    return ret;
  }

  buf_size write(const byte* buf, const buf_size count) override {
    const buf_size ret = write_fd(fd_, buf, count);
    throw_exception_from_negative_value(ret);
    return ret;
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    const int64_t ret64 = lseek_fd(fd_, off, dir);
    throw_exception_from_negative_value(ret64);
    return ret64;
  }

  int flush() override {
    const int ret = fsync_fd(fd_);
    throw_exception_from_negative_value(ret);
    return ret;
  }

  stream_length trunc(const stream_length size) override {
    const int64_t ret64 = ftruncate_fd(fd_, size);
    throw_exception_from_negative_value(ret64);
    return ret64;
  }

 private:
  int fd_;
};
#endif
// stream_interface implementation using FILE*
class cfile_stream final : public stream_interface {
 public:
  explicit cfile_stream(FILE* stream) : stream_(stream) {
    if (!stream) {
      throw null_pointer();
    }
  }

  buf_size read(byte* buf, const buf_size count) override {
    const buf_size ret = fread_file_fp(stream_, buf, count);
    throw_exception_from_negative_value(ret);
    // short-read ok:
    return ret;
  }

  buf_size write(const byte* buf, const buf_size count) override {
    const buf_size ret = fwrite_file_fp(stream_, buf, count);
    throw_exception_from_negative_value(ret);
    // non-negative:
    return ret;
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    const int ret32 = fseek_file_fp(stream_, off, dir);
    throw_exception_from_negative_value(ret32);
    const int64_t ret64 = ftell_file_fp(stream_);
    throw_exception_from_negative_value(ret64);
    return ret64;
  }

  int flush() override {
    const int ret = fflush_file_fp(stream_);
    throw_exception_from_negative_value(ret);
    return ret;
  }

  stream_length trunc(const stream_length size) override {
    const int64_t ret64 = ftruncate_file_fp(stream_, size);
    throw_exception_from_negative_value(ret64);
    return ret64;
  }

 private:
  FILE* stream_;
};

// stream_interface implementation using std::streambuf
class stream_streambuf final : public stream_interface {
 public:
  explicit stream_streambuf(std::streambuf* stream) : stream_(stream) {}

  buf_size read(byte* buf, const buf_size count) override {
    buf_size n = 0;
    for (; n < count; ++n) {
      const int c = stream_->sbumpc();
      if (c == std::char_traits<char>::eof()) {
        break;
      }
      buf[n] = static_cast<char>(c);
    }
    return n;
  }

  buf_size write(const byte* buf, const buf_size count) override {
    buf_size n = 0;
    for (; n < count; ++n) {
      if (stream_->sputc(static_cast<char>(buf[n])) ==
          std::char_traits<char>::eof()) {
        break;
      }
    }
    return n;
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    std::ios_base::seekdir sd;
    switch (dir) {
      case seek_dirs::seek_beg:
        sd = std::ios_base::beg;
        break;
      case seek_dirs::seek_cur:
        sd = std::ios_base::cur;
        break;
      case seek_dirs::seek_end:
        sd = std::ios_base::end;
        break;
      default:
        return -1;
    }
    const auto pos =
        stream_->pubseekoff(off, sd, std::ios_base::in | std::ios_base::out);
    return pos == std::streampos(-1) ? -1 : static_cast<stream_offset>(pos);
  }

  int flush() override { return stream_->pubsync(); }

  stream_length trunc(stream_length) override { throw not_supported(); }

 private:
  std::streambuf* stream_;
};
}  // namespace cxx11
