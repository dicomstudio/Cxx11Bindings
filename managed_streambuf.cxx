#include <iostream>

#ifdef __GNUC__
#define CXX11_SHARP_EXPORT __attribute__((visibility("default")))
#else
#define CXX11_SHARP_EXPORT __declspec(dllexport)
#endif

extern "C" {
typedef int (*read_func)();
typedef int (*write_func)(int);
}

namespace {
/* managed buffer API for system.io.stream. No allocation done, simply provide
 * access to the managed buffer memory */
class managed_buffer {
  read_func read_func_;
  write_func write_func_;
  char* data_;
  int size_;

 public:
  explicit managed_buffer(const read_func read_func,
                          const write_func write_func, char* data,
                          const int size)
      : read_func_(read_func),
        write_func_(write_func),
        data_(data),
        size_(size) {
    if (size < 1) {
      throw std::runtime_error("invalid size");
    }
  }

  int size() const { return size_; }
  char* data() const { return data_; }

  int read() const {
    const int ret = (*read_func_)();
    return ret;
  }

  int write(const int count) const {
    const int ret = (*write_func_)(count);
    return ret;
  }
};

// https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
// https://stackoverflow.com/questions/22116158/whats-wrong-with-this-stream-buffer
class managed_streambuf final : public std::streambuf {
  managed_buffer buffer_;

 public:
  explicit managed_streambuf(const managed_buffer& buffer) : buffer_(buffer) {
    // -1 trick:
    this->setp(this->buffer_.data(),
               this->buffer_.data() + this->buffer_.size() - 1);
  }

  ~managed_streambuf() override {
    try {
      sync();
    } catch (...) {
      // ignore
    }
  }

 private:
  int_type overflow(const int_type i) override {
    if (!traits_type::eq_int_type(i, traits_type::eof())) {
      // see -1 trick in cstor:
      *pptr() = traits_type::to_char_type(i);
      pbump(1);

      if (sync_impl()) {
        // pbump(-(pptr() - pbase()));
        pbump(-this->buffer_.size());
        return i;
      }
      return traits_type::eof();
    }
    return traits_type::not_eof(i);
  }

  int sync() override { return sync_impl() ? 0 : -1; }

  // helper:
  bool sync_impl() const {
    const int num = static_cast<int>(pptr() - pbase());
    const int size = this->buffer_.write(num);
    if (size < 0) {
      throw std::runtime_error("invalid write");
    }
    return size == num;
  }

  int underflow() override {
    if (this->gptr() == this->egptr()) {
      const int size = this->buffer_.read();
      if (size < 0) {
        throw std::runtime_error("invalid read");
      }
      this->setg(this->buffer_.data(), this->buffer_.data(),
                 this->buffer_.data() + size);
    }
    return this->gptr() == this->egptr()
               ? std::char_traits<char>::eof()
               : std::char_traits<char>::to_int_type(*this->gptr());
  }
};
}  // namespace

extern "C" {
struct std_streambuf;

CXX11_SHARP_EXPORT std_streambuf* create_managed_streambuf(
    const read_func read_func, const write_func write_func, char* data,
    const int size) {
  try {
    return reinterpret_cast<std_streambuf*>(new managed_streambuf(
        managed_buffer(read_func, write_func, data, size)));
  } catch (...) {
    return nullptr;
  }
}

CXX11_SHARP_EXPORT void delete_managed_streambuf(std_streambuf* streambuf) {
  delete reinterpret_cast<managed_streambuf*>(streambuf);
}

/* bool is non-blittable type, do not use in API
 * https://learn.microsoft.com/en-us/dotnet/framework/interop/blittable-and-non-blittable-types
 * https://stackoverflow.com/questions/4608876/c-sharp-dllimport-with-c-boolean-function-not-returning-correctly
 */
CXX11_SHARP_EXPORT int copy_to_managed_streambuf(std_streambuf* src_streambuf,
                                                 std_streambuf* dst_streambuf) {
  try {
    std::streambuf* src = reinterpret_cast<std::streambuf*>(src_streambuf);
    std::streambuf* dst = reinterpret_cast<std::streambuf*>(dst_streambuf);
    std::ostream os(dst);
    os << src;
    os.flush();
    // std::cout.exceptions ( std::ios::failbit | std::ios::badbit );
    return os.good() ? 0 : 1;  // no-error
  } catch (...) {
    return 1;
  }
}
}
