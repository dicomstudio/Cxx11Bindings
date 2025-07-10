#include "public.h"

#include <iostream>  // FIXME: streambuf

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
    if (size <= 1) {
      throw std::runtime_error("invalid size");
    }
  }

  char* data() const { return data_; }
  int size() const { return size_; }

  int read() const {
    const int ret = (*read_func_)();
    return ret;
  }

  // write is not const because it modifies the buffer (C# side)
  int write(const int count) /* const */ {
    const int ret = (*write_func_)(count);
    return ret;
  }
};

// https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
// https://stackoverflow.com/questions/22116158/whats-wrong-with-this-stream-buffer
class managed_streambuf final : public std::streambuf {
  managed_buffer buffer_;
  flush_func flush_func_;
  seek_func seek_func_;

 public:
  explicit managed_streambuf(const managed_buffer& buffer,
                             flush_func flush_func, seek_func seek_func)

      : buffer_(buffer), flush_func_(flush_func), seek_func_(seek_func) {
    // -1 trick:
    this->setp(this->buffer_.data(),
               this->buffer_.data() + this->buffer_.size() - 1);
  }

  ~managed_streambuf() override { sync(); }

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

  // O on success, -1 on error
  int sync() override {
    if (!sync_impl()) {
      return -1;
    }
    // bubble up to managed layer:
    const int ret = (*flush_func_)();
    // flush_func_ is: -1 on error, 0 on success:
    return ret;
  }

  // helper:
  bool sync_impl() {
    const int num = static_cast<int>(pptr() - pbase());
    const int size = this->buffer_.write(num);
    if (size < 0) {
      throw std::ios_base::failure("there");
    }
    return size == num;
  }

  pos_type seekoff(const off_type off, const std::ios_base::seekdir way,
                   std::ios_base::openmode which) override {
    const long ret = (*seek_func_)(static_cast<long>(off), way);
    return ret;
  }

  int_type underflow() override {
    if (this->gptr() == this->egptr()) {
      const int size = this->buffer_.read();
      if (size < 0) {
        // https://developercommunity.visualstudio.com/t/Exception-from-streambuf-should-be-caugh/10555755
        // https://stackoverflow.com/a/77704741/136285
        // https://github.com/microsoft/STL/issues/4322
        throw std::ios_base::failure("here");
      }
      this->setg(this->buffer_.data(), this->buffer_.data(),
                 this->buffer_.data() + size);
    }
    return this->gptr() == this->egptr()
               ? traits_type::eof()
               : traits_type::to_int_type(*this->gptr());
  }
};
}  // namespace

extern "C" {
std_streambuf* cxx11_managed_streambuf_create(const read_func read_func,
                                              const write_func write_func,
                                              const flush_func flush_func,
                                              const seek_func seek_func,
                                              char* data, const int size) {
  try {
    return reinterpret_cast<std_streambuf*>(
        new managed_streambuf(managed_buffer(read_func, write_func, data, size),
                              flush_func, seek_func));
  } catch (...) {
    return nullptr;
  }
}

void cxx11_managed_streambuf_delete(std_streambuf* streambuf) {
  delete reinterpret_cast<managed_streambuf*>(streambuf);
}

/* bool is non-blittable type, do not use in API
 * https://learn.microsoft.com/en-us/dotnet/framework/interop/blittable-and-non-blittable-types
 * https://stackoverflow.com/questions/4608876/c-sharp-dllimport-with-c-boolean-function-not-returning-correctly
 */
int cxx11_managed_streambuf_read_into(std_streambuf* src_streambuf,
                                      char* buffer, const int count) {
  std::streambuf* src = reinterpret_cast<std::streambuf*>(src_streambuf);

  try {
    const int ret = static_cast<int>(src->sgetn(buffer, count));
    return ret;
  } catch (...) {
    return -1;
  }
}

int cxx11_managed_streambuf_write_into(std_streambuf* dst_streambuf,
                                       const char* buffer, const int count) {
  std::streambuf* dst = reinterpret_cast<std::streambuf*>(dst_streambuf);
  try {
    const int ret = static_cast<int>(dst->sputn(buffer, count));
    return ret;
  } catch (...) {
    return -1;
  }
}

/* bool is non-blittable type, do not use in API
 * https://learn.microsoft.com/en-us/dotnet/framework/interop/blittable-and-non-blittable-types
 * https://stackoverflow.com/questions/4608876/c-sharp-dllimport-with-c-boolean-function-not-returning-correctly
 */
int cxx11_managed_streambuf_flush(std_streambuf* dst_streambuf) {
  std::streambuf* dst = reinterpret_cast<std::streambuf*>(dst_streambuf);
  const int sync = dst->pubsync();
  return sync;
}

long cxx11_managed_streambuf_test_size(std_streambuf* src_streambuf) {
  const auto src = reinterpret_cast<std::streambuf*>(src_streambuf);
  const std::streampos end_pos =
      src->pubseekoff(0, std::ios_base::end, std::ios_base::in);
  // non-seekable stream:
  if (end_pos == std::streampos(-1)) {
    return -1;
  }

  const std::streampos pos =
      src->pubseekoff(0, std::ios_base::beg, std::ios_base::in);
  // wotsit ?
  if (pos == std::streampos(-1)) {
    return -2;
  }
  return static_cast<long>(end_pos);
}
}
