#pragma once
#include "cxx11bindings.hxx"

#include <streambuf>

namespace cxx11 {
template <int N = 4096>
class simple_streambuf final : public std::streambuf {
  stream_interface* stream_;
  char buffer_[N];

 public:
  simple_streambuf(const simple_streambuf& other) = delete;
  simple_streambuf(simple_streambuf&& other) noexcept = delete;
  simple_streambuf& operator=(const simple_streambuf& other) = delete;
  simple_streambuf& operator=(simple_streambuf&& other) noexcept = delete;

  explicit simple_streambuf(stream_interface* stream = nullptr)
      : stream_(stream) {
    if (!stream_) {
      throw null_pointer();
    }
    setg(buffer_, buffer_, buffer_);
    setp(buffer_, buffer_ + sizeof(buffer_));
  }

  ~simple_streambuf() override {
    if (stream_) {
      close();
    }
  }

#if 0
    bool open(const char* filename, const char* mode) {
        file_ = std::fopen(filename, mode);
        return file_ != nullptr;
    }

    void close() {
        sync();
        if (file_) std::fclose(file_);
        file_ = nullptr;
    }
#else
  void close() {
    sync();
    // FIXME: what if flush returns error?
    if (stream_) {
      stream_->flush();
    }
    stream_ = nullptr;
  }
#endif

 protected:
  int_type underflow() override {
    if (!stream_) {
      throw null_pointer();
    }
#if 0
    size_t n = std::fread(buffer_, 1, sizeof(buffer_), file_);
#else
    auto n = stream_->read(reinterpret_cast<byte*>(buffer_), sizeof(buffer_));
    throw_exception_from_int(n, "read error");
#endif
    if (n == 0) {
      return traits_type::eof();
    }
    setg(buffer_, buffer_, buffer_ + n);
    return traits_type::to_int_type(*gptr());
  }

  int_type overflow(const int_type ch = traits_type::eof()) override {
    if (!stream_) {
      throw null_pointer();
    }
    if (pptr() == pbase()) {
      return traits_type::eof();
    }
#if 0
    size_t n = pptr() - pbase();
        size_t written = std::fwrite(pbase(), 1, n, file_);
#else
    auto n = pptr() - pbase();
    auto written = stream_->write(reinterpret_cast<byte*>(pbase()), n);
    throw_exception_from_int(written, "write error");
#endif
    setp(buffer_, buffer_ + sizeof(buffer_));
    if (ch != traits_type::eof()) {
      *pptr() = traits_type::to_char_type(ch);
      pbump(1);
    }
    return written == n ? ch : traits_type::eof();
  }

  int sync() override {
    if (!stream_) {
      throw null_pointer();
    }
    return stream_->flush();
  }

  std::streamsize xsputn(const char_type* s, const std::streamsize n) override {
    // by default sputn is optimized for small writes, we must implement this to
    // trigger edge cases
    if (!stream_) {
      throw null_pointer();
    }
    const auto written = stream_->write(reinterpret_cast<const byte*>(s), n);
    throw_exception_from_int(written, "write error 2");
    if (written > 0) {
      pbump(static_cast<int>(written));
    }
    return written;
  }

  pos_type seekoff(const off_type off, const std::ios_base::seekdir way,
                   const std::ios_base::openmode which) override {
    if (!stream_) {
      throw null_pointer();
    }
    (void)which;
#if 0
    int whence;
    if (way == std::ios_base::beg) whence = SEEK_SET;
    else if (way == std::ios_base::cur) whence = SEEK_CUR;
    else if (way == std::ios_base::end) whence = SEEK_END;
    else
      throw std::ios_base::failure("invalid dir");
        if (std::fseek(file_, off, whence) != 0) return pos_type(-1);
        return std::ftell(file_);
#else
    int whence;
    if (way == std::ios_base::beg) {
      whence = seek_dirs::seek_beg;
    } else if (way == std::ios_base::cur) {
      whence = seek_dirs::seek_cur;
    } else if (way == std::ios_base::end) {
      whence = seek_dirs::seek_end;
    } else {
      // throw std::invalid_argument("invalid way");
      return -1;
    }
    const auto pos = stream_->seek(off, whence);
    // throw_exception_from_long_long(pos, "seekoff error");
    //  must return {-1} if seek operation fails
    return pos;
#endif
  }

  pos_type seekpos(const pos_type pos,
                   const std::ios_base::openmode which) override {
#if 0
        if (std::fseek(file_, pos, SEEK_SET) != 0) return pos_type(-1);
        return std::ftell(file_);
#else
    const auto new_pos = stream_->seek(pos, seek_dirs::seek_beg);
    // throw_exception_from_long_long(new_pos, "seekpos error");
    // must return {-1} if seek operation fails
    return new_pos;
#endif
  }
};

// no internal buffering, rely on DefaultStreamInterface implementation detail
// simply forwards calls to the DefaultStreamInterface methods
typedef simple_streambuf<> default_streambuf;
typedef simple_streambuf<1> nobuffer_streambuf;
int truncate_file_fp(FILE* fp, int64_t new_size);
}  // namespace cxx11
