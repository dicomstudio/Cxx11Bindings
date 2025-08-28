#ifndef CXX11_BINDINGS_HXX
#define CXX11_BINDINGS_HXX

#include "cxx11bindings.h"
#include "cxx11exceptions.hxx"

#include <algorithm>  // std::min
#include <cstring>    // memmove
#include <vector>

namespace cxx11 {
class stream_interface {
 public:
  stream_interface(const stream_interface& other) = delete;
  stream_interface(stream_interface&& other) noexcept = delete;
  stream_interface& operator=(const stream_interface& other) = delete;
  stream_interface& operator=(stream_interface&& other) noexcept = delete;

  stream_interface() = default;
  virtual ~stream_interface() = default;

  virtual buf_size read(byte* buf, buf_size count) = 0;
  virtual buf_size write(const byte* buf, buf_size count) = 0;
  virtual stream_offset seek(stream_offset off, seek_dir dir) = 0;
  virtual int flush() = 0;

  virtual stream_length trunc(stream_length) {
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }
};

class c_stream final : public stream_interface {
  c11_stream* c11_stream_;

 public:
  c_stream(const c_stream& other) = delete;
  c_stream(c_stream&& other) noexcept = delete;
  c_stream& operator=(const c_stream& other) = delete;
  c_stream& operator=(c_stream&& other) noexcept = delete;

  explicit c_stream(c11_stream* c11_stream) : stream_interface() {
    c11_stream_ = c11_stream;
  }

  ~c_stream() override = default;

  buf_size read(byte* buf, const buf_size count) override {
    if (c11_stream_->read) {
      return c11_stream_->read(buf, count);
    }
    return static_cast<buf_size>(CxxExceptionCode::NotImplemented);
  }

  buf_size write(const byte* buf, const buf_size count) override {
    if (c11_stream_->write) {
      return c11_stream_->write(buf, count);
    }
    return static_cast<buf_size>(CxxExceptionCode::NotImplemented);
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    if (c11_stream_->seek) {
      return c11_stream_->seek(off, dir);
    }
    return static_cast<stream_offset>(CxxExceptionCode::NotImplemented);
  }

  int flush() override {
    if (c11_stream_->flush) {
      return c11_stream_->flush();
    }
    return static_cast<int>(CxxExceptionCode::NotImplemented);
  }

  stream_length trunc(const stream_length size) override {
    if (c11_stream_->trunc) {
      return c11_stream_->trunc(size);
    }
    return static_cast<stream_length>(CxxExceptionCode::NotImplemented);
  }
};

// Outside class to cope with c++11 standard:
static constexpr std::size_t put_back_size = 8;
// basic default streambuf implementation using c11_stream
class basic_streambuf final : public std::streambuf {
 public:
  explicit basic_streambuf(stream_interface* f,
                           const std::size_t buff_sz = 4096)
      : stream_(f), buffer_(buff_sz + put_back_size) {
    char* base = buffer_.data();
    setg(base + put_back_size, base + put_back_size, base + put_back_size);
    setp(base, base + buffer_.size());
  }

  ~basic_streambuf() override {
    sync();  // flush pending output
  }

 protected:
  // Input
  int_type underflow() override {
    if (!stream_) return traits_type::eof();

    // putback space
    std::size_t putback = gptr() - eback();
    putback = std::min(putback, put_back_size);

    std::memmove(buffer_.data() + (put_back_size - putback), gptr() - putback,
                 putback);

    const buf_size n =
        stream_->read(reinterpret_cast<byte*>(buffer_.data() + put_back_size),
                      static_cast<buf_size>(buffer_.size() - put_back_size));
    if (n < 0) {
      // https://developercommunity.visualstudio.com/t/Exception-from-streambuf-should-be-caugh/10555755
      // https://stackoverflow.com/a/77704741/136285
      // https://github.com/microsoft/STL/issues/4322
      throw std::ios_base::failure("stream read error.");
    }
    if (n == 0) return traits_type::eof();

    setg(buffer_.data() + (put_back_size - putback),
         buffer_.data() + put_back_size, buffer_.data() + put_back_size + n);

    return traits_type::to_int_type(*gptr());
  }

  // Output
  int_type overflow(const int_type ch) override {
    if (!stream_) return traits_type::eof();

    if (pptr() != pbase()) {
      if (flush_buffer() == traits_type::eof()) return traits_type::eof();
    }

    if (!traits_type::eq_int_type(ch, traits_type::eof())) {
      *pptr() = traits_type::to_char_type(ch);
      pbump(1);
    }

    return traits_type::not_eof(ch);
  }

  int sync() override { return flush_buffer() == traits_type::eof() ? -1 : 0; }

  // Seeking
  pos_type seekoff(off_type off, const std::ios_base::seekdir way,
                   const std::ios_base::openmode which) override {
    if (!stream_) return {-1};

    // Flush output buffer before seeking
    if (which & std::ios_base::out) {
      if (sync() == -1) return {-1};
    }

    // Calculate origin
    seek_dir origin;
    switch (way) {
      case std::ios_base::beg:
        origin = seek_dirs::seek_beg;
        break;
      case std::ios_base::cur:
        origin = seek_dirs::seek_cur;
        break;
      case std::ios_base::end:
        origin = seek_dirs::seek_end;
        break;
      default:
        return {-1};
    }

    // If seeking relative to current in input mode, adjust by unread bytes
    if (which & std::ios_base::in && way == std::ios_base::cur) {
      off -= egptr() - gptr();
    }

    const stream_offset pos = stream_->seek(off, origin);

    // Invalidate buffer after seek
    setg(buffer_.data() + put_back_size, buffer_.data() + put_back_size,
         buffer_.data() + put_back_size);
    setp(buffer_.data(), buffer_.data() + buffer_.size());

    if (pos < 0) return {-1};
    return {pos};
  }

  pos_type seekpos(const pos_type sp,
                   const std::ios_base::openmode which) override {
    return seekoff(sp, std::ios_base::beg, which);
  }

 private:
  stream_interface* stream_;
  std::vector<char> buffer_;

  int flush_buffer() {
    const std::ptrdiff_t n = pptr() - pbase();
    if (n > 0) {
      const buf_size ret = stream_->write(
          reinterpret_cast<const byte*>(pbase()), static_cast<buf_size>(n));
      if (ret < 0) {
        throw std::ios_base::failure("flush_buffer write error.");
      }
      if (ret != static_cast<buf_size>(n)) return traits_type::eof();
      pbump(static_cast<int>(-n));
    }
    return 0;
  }
};
}  // namespace cxx11

#endif  // CXX11_BINDINGS_HXX
