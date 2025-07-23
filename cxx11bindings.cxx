#include "cxx11bindings.h"
#include "cxx11bindings.hxx"

namespace cxx11 {
/* managed buffer API for system.io.stream. No allocation done, simply provides
 * access to the managed buffer memory */
class virtual_buffer {
  byte* data_;
  size count_;

 public:
  explicit virtual_buffer(byte* data, const size count)
      : data_(data), count_(count) {
    if (count < 1 || count > 0x7ffff000) {
      throw std::runtime_error("invalid size");
    }
  }

  char* data() const { return reinterpret_cast<char*>(data_); }
  size count() const { return count_; }
};

// https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
// https://stackoverflow.com/questions/22116158/whats-wrong-with-this-stream-buffer
class streambuf final : public std::streambuf {
  virtual_buffer buffer_;
  read_fn read_;
  write_fn write_;
  seek_fn seek_;
  flush_fn flush_;

 public:
  streambuf(const streambuf& other) = delete;
  streambuf(streambuf&& other) noexcept = delete;
  streambuf& operator=(const streambuf& other) = delete;
  streambuf& operator=(streambuf&& other) noexcept = delete;

  explicit streambuf(const virtual_buffer& buffer, const read_fn read,
                     const write_fn write, const seek_fn seek,
                     const flush_fn flush)
      : buffer_(buffer),
        read_(read),
        write_(write),
        seek_(seek),
        flush_(flush) {
    // -1 trick:
    this->setp(this->buffer_.data(),
               this->buffer_.data() + this->buffer_.count() - 1);
  }

  ~streambuf() override { sync(); }

 private:
  int_type overflow(const int_type i) override {
    if (!traits_type::eq_int_type(i, traits_type::eof())) {
      // see -1 trick in cstor:
      *pptr() = traits_type::to_char_type(i);
      pbump(1);

      if (sync_impl()) {
        // pbump(-(pptr() - pbase()));
        pbump(-static_cast<int>(this->buffer_.count()));
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
    const int ret = flush_();
    // flush_func_ is: -1 on error, 0 on success:
    return ret;
  }

  // helper:
  bool sync_impl() {
    const int num = static_cast<int>(pptr() - pbase());
    // const int size = this->buffer_.write(num);
    const auto buf = reinterpret_cast<const byte*>(this->buffer_.data());
    const int size = write_(buf, num);
    if (size < 0) {
      throw std::ios_base::failure("there");
    }
    return size == num;
  }

  // Override seekoff for seeking by offset
  pos_type seekoff(const off_type off, const std::ios_base::seekdir dir,
                   std::ios_base::openmode which) override {
    const offset ret = seek_(static_cast<offset>(off), dir);
    return ret;
  }

  // Override seekpos for seeking to an absolute position
  pos_type seekpos(const pos_type pos, std::ios_base::openmode which) override {
    const offset ret = seek_(static_cast<offset>(pos), std::ios_base::beg);
    return ret;
  }

  // fetch more data:
  int_type underflow() override {
    if (this->gptr() == this->egptr()) {
      // const int size = this->buffer_.read();
      const auto buf = reinterpret_cast<byte*>(this->buffer_.data());
      const auto count = this->buffer_.count();
      const auto size = read_(buf, count);
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
}  // namespace cxx11
extern "C" {
c11_stream* c11_stream_create(const read_fn read, const write_fn write,
                              const seek_fn seek, const flush_fn flush,
                              const trunc_fn trunc) {
  try {
    return reinterpret_cast<c11_stream*>(
        new (std::nothrow) cxx11::c_stream(read, write, seek, flush, trunc));
  } catch (...) {
    // return reinterpret_cast<c11_stream*>(-1);
    return nullptr;
  }
}

int c11_stream_delete(c11_stream* c11_stream) {
  try {
    delete reinterpret_cast<cxx11::c_stream*>(c11_stream);
    return 0;
  } catch (...) {
    // FIXME
    return -1;
  }
}

// std::streambuf with c++11 ABI:
cxx11_streambuf* cxx11_streambuf_create(read_fn read, write_fn write,
                                        seek_fn seek, flush_fn flush) {
  try {
    // return reinterpret_cast<cxx11_streambuf*>(new(std::nothrow)
    // cxx11::streambuf(read, write, seek, flush));
  } catch (...) {
    // return reinterpret_cast<c11_stream*>(-1);
  }
  return nullptr;
}

cxx11_streambuf* cxx11_streambuf_create_buffer(const read_fn read,
                                               const write_fn write,
                                               const seek_fn seek,
                                               const flush_fn flush, byte* buf,
                                               const size count) {
  try {
    return reinterpret_cast<cxx11_streambuf*>(
        new (std::nothrow) cxx11::streambuf(cxx11::virtual_buffer(buf, count),
                                            read, write, seek, flush));
  } catch (...) {
    // return reinterpret_cast<c11_stream*>(-1);
    return nullptr;
  }
}

int cxx11_streambuf_delete(cxx11_streambuf* cxx11_streambuf) {
  try {
    delete reinterpret_cast<cxx11::streambuf*>(cxx11_streambuf);
    return 0;
  } catch (...) {
    // FIXME
    return -1;
  }
}
}  // extern "C"
