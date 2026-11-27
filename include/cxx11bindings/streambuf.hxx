#ifndef CXX11_BINDINGS_STREAMBUF_HXX
#define CXX11_BINDINGS_STREAMBUF_HXX

#include <cxx11bindings/stream.h>

#include <algorithm>  // std::min
#include <cassert>
#include <complex>
#include <cstddef>
#include <cstring>  // memmove
#include <limits>
#include <stdexcept>
#include <streambuf>
#include <string>
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
  InvalidArgument = C11_E_INVALIDARG,
  ArgumentOutOfRange = C11_E_ARGUMENTOUTOFRANGE,
  ObjectDisposed = C11_E_OBJECTDISPOSED,
  IOException = C11_E_IO,
  InvalidOperation = C11_E_INVALIDOPERATION,
  Unauthorized = C11_E_UNAUTHORIZED,
  OutOfMemory = C11_E_OUTOFMEMORY,
  Timeout = C11_E_TIMEOUT,
  Canceled = C11_E_CANCELED,
  FileNotFound = C11_E_FILENOTFOUND,
  DirectoryNotFound = C11_E_DIRECTORYNOTFOUND,
  PathTooLong = C11_E_PATHTOOLONG,
  EndOfStream = C11_E_ENDOFSTREAM,
  ArgumentNull = C11_E_ARGUMENTNULL,
  NotImplemented = C11_E_NOTIMPLEMENTED,
  Unmapped = C11_E_UNMAPPED
};

// code() classifies unknown values as Unmapped; raw_code() preserves them.
class stream_exception final : public std::runtime_error {
 public:
  explicit stream_exception(ErrorCode code)
      : stream_exception(static_cast<int64_t>(code)) {}

  explicit stream_exception(int64_t code)
      : std::runtime_error("C11 stream error code " +
                           std::to_string(code)),
        code_(code) {}

  ErrorCode code() const noexcept {
    if (code_ >= static_cast<int64_t>(C11_E_POINTER) &&
        code_ <= static_cast<int64_t>(C11_E_UNMAPPED)) {
      return static_cast<ErrorCode>(code_);
    }
    return ErrorCode::Unmapped;
  }
  int64_t raw_code() const noexcept { return code_; }

 private:
  int64_t code_;
};

static inline void throw_exception_from_enum(const ErrorCode err_code) {
  throw stream_exception(err_code);
}

template <typename T>
static inline void throw_exception_from_negative_value(T value) {
  if (value < 0) {
    throw stream_exception(static_cast<int64_t>(value));
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
    throw_exception_from_negative_value(value);
    return value;
  }

  buf_size write(const byte* buf, const buf_size count) override {
    const auto value = c11_stream_write(c11_stream_, buf, count);
    throw_exception_from_negative_value(value);
    return value;
  }

  stream_offset seek(const stream_offset off, const seek_dir dir) override {
    const auto value = c11_stream_seek(c11_stream_, off, dir);
    throw_exception_from_negative_value(value);
    return value;
  }

  int flush() override {
    const auto value = c11_stream_flush(c11_stream_);
    throw_exception_from_negative_value(value);
    return value;
  }

  stream_length trunc(const stream_length size) override {
    const auto value = c11_stream_trunc(c11_stream_, size);
    throw_exception_from_negative_value(value);
    return value;
  }
};

// Outside class to cope with c++11 standard:
static constexpr std::size_t put_back_size = 8;

// basic default streambuf implementation using c11_stream
class basic_streambuf final : public std::streambuf {
 public:
  // buff_sz must be positive and leave room for putback within the supported
  // callback and pointer ranges; invalid sizes throw before allocating.
  explicit basic_streambuf(stream_interface& f,
                           const std::size_t buff_sz = 4096)
      : stream_(f), buffer_(checked_buffer_size(buff_sz)) {
    char* base = buffer_.data();
    setg(base + put_back_size, base + put_back_size, base + put_back_size);
    setp(base, base + buffer_.size());
  }

  // Destruction is best-effort; call pubsync() explicitly to detect errors.
  ~basic_streambuf() noexcept override {
    try {
      sync();
    } catch (...) {
      // An exception cannot be reported safely from a destructor.
    }
  }

 protected:
  // Input
  int_type underflow() override {
    // putback space
    std::size_t putback = gptr() - eback();
    putback = std::min(putback, put_back_size);

    std::memmove(buffer_.data() + (put_back_size - putback), gptr() - putback,
                 putback);
    setg(buffer_.data() + (put_back_size - putback),
         buffer_.data() + put_back_size, buffer_.data() + put_back_size);

    // ->read() call may throw, but this is legitimate behavior:
    // https://developercommunity.visualstudio.com/t/Exception-from-streambuf-should-be-caugh/10555755
    // https://stackoverflow.com/a/77704741/136285
    // https://github.com/microsoft/STL/issues/4322
    const buf_size count = static_cast<buf_size>(buffer_.size() - put_back_size);
    const buf_size n = stream_.read(
        reinterpret_cast<byte*>(buffer_.data() + put_back_size), count);
    throw_exception_from_negative_value(n);
    if (n > count) {
      throw stream_exception(ErrorCode::InvalidOperation);
    }
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
    // Both input and output seeks change the shared underlying position.
    if (sync() == -1) {
      return {-1};
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

    // A failed or throwing seek may still change the underlying position.
    setg(buffer_.data() + put_back_size, buffer_.data() + put_back_size,
         buffer_.data() + put_back_size);
    const stream_offset pos = stream_.seek(off, origin);

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
  static std::size_t checked_buffer_size(const std::size_t size) {
    if (size == 0) {
      throw std::invalid_argument("stream buffer size must be positive");
    }
    const uintmax_t max_total = (std::min)(
        (std::min)(static_cast<uintmax_t>((std::numeric_limits<buf_size>::max)()),
                   static_cast<uintmax_t>((std::numeric_limits<int>::max)())),
        (std::min)(static_cast<uintmax_t>((std::numeric_limits<std::ptrdiff_t>::max)()),
                   static_cast<uintmax_t>((std::numeric_limits<std::size_t>::max)())));
    if (static_cast<uintmax_t>(size) > max_total - put_back_size) {
      throw std::length_error("stream buffer size exceeds supported range");
    }
    return size + put_back_size;
  }

  stream_interface& stream_;
  std::vector<char> buffer_;

  int flush_buffer() {
    while (pptr() != pbase()) {
      const std::ptrdiff_t n = pptr() - pbase();
      const buf_size ret = stream_.write(reinterpret_cast<const byte*>(pbase()),
                                         static_cast<buf_size>(n));
      if (ret <= 0 || ret > n) {
        return traits_type::eof();
      }
      std::memmove(pbase(), pbase() + ret, static_cast<std::size_t>(n - ret));
      pbump(-ret);
    }
    return 0;
  }
};
}  // namespace cxx11

#endif  // CXX11_BINDINGS_STREAMBUF_HXX
