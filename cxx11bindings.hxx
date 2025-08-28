#ifndef CXX11_BINDINGS_HXX
#define CXX11_BINDINGS_HXX

#include "cxx11bindings.h"

#include <algorithm>  // std::min
#include <cassert>
#include <complex>
#include <cstring>  // memmove
#include <stdexcept>
#include <streambuf>
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
  virtual stream_length trunc(stream_length) = 0;
};

// --- exceptions ---
// Define an enum for C++ exception
enum class ErrorCode : int {
  NullPointer = C11_E_POINTER,
  NotSupported = C11_E_NOTSUPPORTED,
  NotImplemented = C11_E_NOTIMPL,
  ArgumentException = C11_E_INVALIDARG,
};

// Custom exception class
class null_pointer final : public std::exception {
 public:
  const char* what() const noexcept override {
    return "Attempted to dereference a null pointer.";
  }

  null_pointer() = default;
};

class not_supported final : public std::runtime_error {
 public:
  explicit not_supported(const std::string& message = "Function not supported")
      : std::runtime_error(message) {}
};

class not_implemented final : public std::logic_error {
 public:
  explicit not_implemented(
      const std::string& message = "Function not yet implemented")
      : std::logic_error(message) {}
};

class argument_exception final : public std::runtime_error {
 public:
  explicit argument_exception(const std::string& message = "Argument error")
      : std::runtime_error(message) {}
};

[[noreturn]] static inline void throw_exception_from_enum(
    const ErrorCode err_code) {
  switch (err_code) {
    case ErrorCode::NullPointer:
      throw null_pointer();
    case ErrorCode::NotSupported:
      throw not_supported();
    case ErrorCode::NotImplemented:
      throw not_implemented();
    case ErrorCode::ArgumentException:
      throw argument_exception();
  }
  assert(0);
}

template <typename T>
static inline void throw_exception_from_value(T value) {
  // FIXME: check int32
  if (value < 0) {
    throw_exception_from_enum(static_cast<ErrorCode>(value));
  }
}

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
    const auto value = c11_stream_read(c11_stream_, buf, count);
    throw_exception_from_value(value);
    return value;
  }

  buf_size write(const byte* buf, const buf_size count) override {
    const auto value = c11_stream_write(c11_stream_, buf, count);
    throw_exception_from_value(value);
    return value;
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    const auto value = c11_stream_seek(c11_stream_, off, dir);
    throw_exception_from_value(value);
    return value;
  }

  int flush() override {
    const auto value = c11_stream_flush(c11_stream_);
    throw_exception_from_value(value);
    return value;
  }

  stream_length trunc(const stream_length size) override {
    const auto value = c11_stream_trunc(c11_stream_, size);
    throw_exception_from_value(value);
    return value;
  }
};

// Outside class to cope with c++11 standard:
static constexpr std::size_t put_back_size = 8;

// basic default streambuf implementation using c11_stream
class basic_streambuf final : public std::streambuf {
 public:
  explicit basic_streambuf(stream_interface& f,
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
    // putback space
    std::size_t putback = gptr() - eback();
    putback = std::min(putback, put_back_size);

    std::memmove(buffer_.data() + (put_back_size - putback), gptr() - putback,
                 putback);

    // ->read() call may throw, but this is legitimate behavior:
    // https://developercommunity.visualstudio.com/t/Exception-from-streambuf-should-be-caugh/10555755
    // https://stackoverflow.com/a/77704741/136285
    // https://github.com/microsoft/STL/issues/4322
    const buf_size n =
        stream_.read(reinterpret_cast<byte*>(buffer_.data() + put_back_size),
                     static_cast<buf_size>(buffer_.size() - put_back_size));
    if (n == 0) {
      return traits_type::eof();
    }

    setg(buffer_.data() + (put_back_size - putback),
         buffer_.data() + put_back_size, buffer_.data() + put_back_size + n);

    return traits_type::to_int_type(*gptr());
  }

  // Output
  int_type overflow(const int_type ch) override {
    if (pptr() != pbase()) {
      if (flush_buffer() == traits_type::eof()) {
        return traits_type::eof();
      }
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
    // Flush output buffer before seeking
    if (which & std::ios_base::out) {
      if (sync() == -1) {
        return {-1};
      }
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

    const stream_offset pos = stream_.seek(off, origin);

    // Invalidate buffer after seek
    setg(buffer_.data() + put_back_size, buffer_.data() + put_back_size,
         buffer_.data() + put_back_size);
    setp(buffer_.data(), buffer_.data() + buffer_.size());

    if (pos < 0) {
      return {-1};
    }
    return {pos};
  }

  pos_type seekpos(const pos_type sp,
                   const std::ios_base::openmode which) override {
    return seekoff(sp, std::ios_base::beg, which);
  }

 private:
  stream_interface& stream_;
  std::vector<char> buffer_;

  int flush_buffer() {
    const std::ptrdiff_t n = pptr() - pbase();
    if (n > 0) {
      const buf_size ret = stream_.write(reinterpret_cast<const byte*>(pbase()),
                                         static_cast<buf_size>(n));
      if (ret != static_cast<buf_size>(n)) {
        return traits_type::eof();
      }
      pbump(static_cast<int>(-n));
    }
    return 0;
  }
};
}  // namespace cxx11

#endif  // CXX11_BINDINGS_HXX
