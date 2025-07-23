#pragma once
#include "cxx11bindings.hxx"
#include <cassert>
#include <ios>
#include <streambuf>
#include <string>

namespace cxx11 {
class streambuf_adapter final : public stream_interface {
 public:
  explicit streambuf_adapter(std::streambuf* stream) : stream_(stream) {}

  int read(byte* buf, const size count) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    int n = 0;
    for (; n < count; ++n) {
      const int c = stream_->sbumpc();
      if (c == std::char_traits<char>::eof()) {
        break;
      }
      buf[n] = static_cast<char>(c);
    }
    return n;
  }

  int write(const byte* buf, const size count) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    int n = 0;
    for (; n < count; ++n) {
      if (stream_->sputc(static_cast<char>(buf[n])) ==
          std::char_traits<char>::eof()) {
        break;
      }
    }
    return n;
  }

  int64_t seek(const int64_t off, const int dir) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
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
    return pos == std::streampos(-1) ? -1 : static_cast<int64_t>(pos);
  }

  int flush() override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    return stream_->pubsync();
  }

 private:
  std::streambuf* stream_;
};

// C-FILE
class cfile_adapter final : public stream_interface {
 public:
  explicit cfile_adapter(FILE* stream) : stream_(stream) {}

  int read(byte* buf, const size count) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    const size_t ret = fread(buf, 1, count, stream_);
    return static_cast<int>(ret);
  }

  int write(const byte* buf, size count) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    const size_t ret = fwrite(buf, 1, count, stream_);
    return static_cast<int>(ret);
  }

  offset seek(const offset off, const seek_dir dir) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    int whence;
    switch (dir) {
      case seek_dirs::seek_beg:
        whence = SEEK_SET;
        break;
      case seek_dirs::seek_cur:
        whence = SEEK_CUR;
        break;
      case seek_dirs::seek_end:
        whence = SEEK_END;
        break;
      default:
        return static_cast<int>(CxxExceptionCode::InvalidArgument);
    }
    const int ret = fseek(stream_, off, whence);
    return ret == -1 ? -1 : static_cast<int64_t>(ftell(stream_));
  }

  int flush() override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    const int ret = fflush(stream_);
    if (ret == 0) {
      return ret;
    }
    // Otherwise, EOF is returned and errno is set to indicate the error.
    // int err = errno;
    // EBADF  stream is not an open stream, or is not open for writing.
    return -1;  // static_cast<int>(ErrorCode::IoFailure);
  }

 private:
  FILE* stream_;
};
}  // namespace cxx11
