// Mostly for internal testing, it provide differnt stream_interface
// implementation using standard API (FILE*, std::streambuf)
#pragma once
#include "cxx11bindings.hxx"
#include <cassert>
#include <ios>
#include <streambuf>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>  // _chsize_s
#else
#include <unistd.h>  // ftruncate
#endif

// Truncate an open FILE* stream to a given size (in bytes).
// Returns 0 on success, -1 on error.
static inline int truncate_file_fp(FILE* fp, int64_t new_size) {
  if (!fp || new_size < 0) {
    return -1;
  }

#ifdef _WIN32
  int fd = _fileno(fp);
  if (fd == -1) {
    return -1;
  }

  if (_chsize_s(fd, new_size) != 0) {
    return -1;
  }
#else
  int fd = fileno(fp);
  if (fd == -1) {
    return -1;
  }

  if (ftruncate(fd, new_size) != 0) {
    return -1;
  }
#endif

  return 0;
}

static inline int fseek_file_fp(FILE* stream, int64_t offset, int whence) {
#ifdef _WIN32
  return _fseeki64(stream, offset, whence);
#else
  return fseeko(stream, offset, whence);
#endif
}

static inline int64_t ftell_file_fp(FILE* stream) {
#ifdef _WIN32
  return _ftelli64(stream);
#else
  return ftello(stream);
#endif
}

namespace cxx11 {
// stream_interface implementation using FILE*
class cfile_stream final : public stream_interface {
 public:
  explicit cfile_stream(FILE* stream) : stream_(stream) {}

  int read(byte* buf, const size count) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    const size_t ret = fread(buf, 1, count, stream_);
    return static_cast<int>(ret);
  }

  int write(const byte* buf, const size count) override {
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
    const int ret = fseek_file_fp(stream_, off, whence);
    return ret == -1 ? -1 : ftell_file_fp(stream_);
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

  int trunc(const length size) override {
    return truncate_file_fp(stream_, size);
  }

 private:
  FILE* stream_;
};

// stream_interface implementation using std::streambuf
class stream_streambuf final : public stream_interface {
 public:
  explicit stream_streambuf(std::streambuf* stream) : stream_(stream) {}

  int read(byte* buf, const size count) override {
    if (!stream_) {
      return static_cast<int>(CxxExceptionCode::NullPointer);
    }
    int n = 0;
    for (; n < static_cast<int>(count); ++n) {
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
    for (; n < static_cast<int>(count); ++n) {
      if (stream_->sputc(static_cast<char>(buf[n])) ==
          std::char_traits<char>::eof()) {
        break;
      }
    }
    return n;
  }

  offset seek(const offset off, const seek_dir dir) override {
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
    return pos == std::streampos(-1) ? -1 : static_cast<offset>(pos);
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
}  // namespace cxx11
